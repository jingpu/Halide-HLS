#include "HalideRuntimeZynq.h"
#include "printer.h"

extern "C" {

typedef struct {
    uint32_t flags;
    void * stack_base;
    size_t stack_size;
    size_t guard_size;
    int32_t sched_policy;
    int32_t sched_priority;
} pthread_attr_t;
typedef long pthread_t;
typedef struct {
    // 48 bytes is enough for a cond on 64-bit and 32-bit systems
    uint64_t _private[6];
} pthread_cond_t;
typedef long pthread_condattr_t;
typedef struct {
    // 64 bytes is enough for a mutex on 64-bit and 32-bit systems
    uint64_t _private[8];
} pthread_mutex_t;
typedef long pthread_mutexattr_t;
extern int pthread_create(pthread_t *thread, pthread_attr_t const * attr,
                          void *(*start_routine)(void *), void * arg);
extern int pthread_join(pthread_t thread, void **retval);
extern int pthread_cond_init(pthread_cond_t *cond, const pthread_condattr_t *attr);
extern int pthread_cond_wait(pthread_cond_t *cond, pthread_mutex_t *mutex);
extern int pthread_cond_signal(pthread_cond_t *cond);
extern int pthread_cond_broadcast(pthread_cond_t *cond);
extern int pthread_cond_destroy(pthread_cond_t *cond);
extern int pthread_mutex_init(pthread_mutex_t *mutex, const pthread_mutexattr_t *attr);
extern int pthread_mutex_lock(pthread_mutex_t *mutex);
extern int pthread_mutex_unlock(pthread_mutex_t *mutex);
extern int pthread_mutex_destroy(pthread_mutex_t *mutex);

extern char *getenv(const char *);
extern unsigned int sleep(unsigned int seconds);
extern int usleep(int);
extern int atoi(const char *);
} // extern "C"

#define MAX_NUM_ACC 32
static int initialized = 0;
static int num_accelerator = 0;
static int accelerator_latency_us = 0;

typedef struct work {
    work *next_job;
    int job_id;
} work;

typedef struct work_queue_t {
  // all fields are protected by this mutex.
  pthread_mutex_t mutex;

  // Singly linked list for job queue
  work head;  // head is dummy node (always valid to read)
  work *tail;

  int next_job_id;

  pthread_cond_t wakeup_workers;
  // Keep track of threads so they can be joined at shutdown
  pthread_t threads[MAX_NUM_ACC];
} work_queue_t;

typedef struct finish_queue_t {
  // all fields are protected by this mutex.
  pthread_mutex_t mutex;

  // Singly linked list for job stack
  work *head;

  pthread_cond_t worker_finished;
} finish_queue_t;

static work_queue_t work_queue;
static finish_queue_t finish_queue;

void print_list(work *head, const char* s) {
  debug(0) << s << " [";
  while (head) {
    debug(0) << head->job_id << ", ";
    head = head->next_job;
  }
  debug(0) << "]\n";
}

void *worker_thread(void *void_arg) {
  pthread_mutex_lock(&work_queue.mutex);
  while (true) {
    if (work_queue.head.next_job == NULL) {
      // wait for a new job
      pthread_cond_wait(&work_queue.wakeup_workers, &work_queue.mutex);
    } else {
      // grab a next job
      work *job = work_queue.head.next_job;
      work_queue.head.next_job = job->next_job;
      if (work_queue.tail == job) {
        work_queue.tail = &work_queue.head;
      }
      //print_list(&work_queue.head, "pop a job");

      // Release the lock and do the task.
      pthread_mutex_unlock(&work_queue.mutex);
      debug(0) << "Worker starts running job " << job->job_id << "\n";
      //sleep(HWACC_LATENCY_SEC);
      usleep(accelerator_latency_us);
      debug(0) << "Worker finished job " << job->job_id << "\n";
      pthread_mutex_lock(&work_queue.mutex);

      pthread_mutex_lock(&finish_queue.mutex);
      // move the job to finish stack
      job->next_job = finish_queue.head;
      finish_queue.head = job;
      print_list(finish_queue.head, "push a job to finish queue");

      pthread_cond_broadcast(&finish_queue.worker_finished);
      pthread_mutex_unlock(&finish_queue.mutex);
    }
  }
  pthread_mutex_unlock(&work_queue.mutex);
  return NULL;
}


static int add_job() {
  pthread_mutex_lock(&work_queue.mutex);

  // make the job
  work *job = (work *)malloc(sizeof(work));
  int job_id = work_queue.next_job_id++;
  job->job_id = job_id;
  job->next_job = NULL;

  // push the job to the queue
  work_queue.tail->next_job = job;
  work_queue.tail = job;
  //print_list(&work_queue.head, "push a job to work queue");

  debug(0) << "Added job" << job->job_id << " to work queue\n";

  // wake up a workers
  pthread_cond_signal(&work_queue.wakeup_workers);

  pthread_mutex_unlock(&work_queue.mutex);
  return job_id;
}

static void wait_job(int job_id) {
  pthread_mutex_lock(&finish_queue.mutex);

  bool found = false;
  while (true) {

    // search through the finish queue
    work dummy = {finish_queue.head, -1};
    work *cur = &dummy, *pre;
    while (cur->next_job) {
      pre = cur;
      cur = cur->next_job;
      if (cur->job_id == job_id) {
        pre->next_job = cur->next_job;
        free(cur);
        found = true;
        break;
      }
    }
    finish_queue.head = dummy.next_job;
    if (found) {
      print_list(finish_queue.head, "pop a job from finish queueu: ");
      break;
    }
    pthread_cond_wait(&finish_queue.worker_finished, &finish_queue.mutex);
  }
  pthread_mutex_unlock(&finish_queue.mutex);
}

static void init_work_queue() {
  pthread_mutex_lock(&work_queue.mutex);
  if (!initialized) {
    //pthread_mutex_init(&work_queue.mutex, NULL);
    pthread_cond_init(&work_queue.wakeup_workers, NULL);
    pthread_cond_init(&finish_queue.worker_finished, NULL);

    work_queue.head.job_id = -1;
    work_queue.head.next_job = NULL;
    work_queue.tail = &work_queue.head;
    work_queue.next_job_id = 0;

    finish_queue.head = NULL;

    for (int i = 0; i < num_accelerator; i++) {
      debug(0) << "Creating thread " << i << "\n";
      pthread_create(work_queue.threads + i, NULL, worker_thread, NULL);
    }
    initialized = 1;
  }
  pthread_mutex_unlock(&work_queue.mutex);
}

static void shutdown_queue() {
  if (!initialized) return;

  for (int i = 0; i < num_accelerator; i++) {
    void *retval;
    pthread_join(work_queue.threads[i], &retval);
  }
  debug(0) << "All threads have quit. Destroying mutex and condition variable.\n";
  pthread_mutex_destroy(&work_queue.mutex);
  pthread_cond_destroy(&work_queue.wakeup_workers);
  pthread_cond_destroy(&finish_queue.worker_finished);
  initialized = 0;
}


extern "C" {


WEAK int halide_zynq_init() {
    if (!num_accelerator) {
      char *num_str = getenv("HL_ZYNQ_NUM_ACC");
      if (num_str) {
        num_accelerator = atoi(num_str);
      } else {
        num_accelerator = 8;
      }
      debug(0) << "num_accelerator = " << num_accelerator << "\n";
    }
    if (!accelerator_latency_us) {
      char *lat_str = getenv("HL_ZYNQ_ACC_LAT");
      if (lat_str) {
        accelerator_latency_us = atoi(lat_str);
      } else {
        accelerator_latency_us = 1000 * 1000;
      }
      debug(0) << "accelerator_latency_us = " << accelerator_latency_us << "\n";
    }

    init_work_queue();
    return 0;
}

WEAK void halide_zynq_free(void *user_context, void *ptr) {
  //debug(1) << "halide_zynq_free\n";
    // do nothing
}

WEAK int halide_zynq_cma_alloc(struct buffer_t *buf) {
  //debug(1) << "halide_zynq_cma_alloc\n";
    halide_zynq_init();

    // Figure out how much memory to allocate for this buffer
    size_t min_idx = 0, max_idx = 0;
    for (int d = 0; d < 4; d++) {
        if (buf->stride[d] > 0) {
            min_idx += buf->min[d] * buf->stride[d];
            max_idx += (buf->min[d] + buf->extent[d] - 1) * buf->stride[d];
        } else {
            max_idx += buf->min[d] * buf->stride[d];
            min_idx += (buf->min[d] + buf->extent[d] - 1) * buf->stride[d];
        }
    }
    size_t total_size = (max_idx - min_idx);
    while (total_size & 0x1f) total_size++;

    buf->host = (uint8_t*) malloc(total_size*buf->elem_size);

    if ((void *) buf->host == (void *) -1) {
        error(NULL) << "malloc failed.\n";
        return -3;
    }
    return 0;
}

WEAK int halide_zynq_cma_free(struct buffer_t *buf) {
  //debug(1) << "halide_zynq_cma_free\n";
    free(buf->host);
    return 0;
}

WEAK int halide_zynq_subimage(const struct buffer_t* image, struct cma_buffer_t* subimage, void *address_of_subimage_origin, int width, int height) {
  //debug(1) << "halide_zynq_subimage\n";
    // do nothing
    return 0;
}

WEAK int halide_zynq_hwacc_launch(struct cma_buffer_t bufs[]) {
    debug(0) << "halide_zynq_hwacc_launch\n";
    return add_job();
}

WEAK int halide_zynq_hwacc_sync(int task_id){
    debug(0) << "halide_zynq_hwacc_sync\n";
    //sleep(1);
    wait_job(task_id);
    return 0;
}

}

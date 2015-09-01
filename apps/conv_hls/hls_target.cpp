#include "hls_target.h"

void hls_target_f3_stream_stencil_stream(
hls::stream<Stencil<uint8_t, 1, 1, 1> > &_clamped_stream_stencil_stream,
hls::stream<Stencil<uint8_t, 1, 1, 1> > &_f3_stream_stencil_stream)
{
 {
  hls::stream<Stencil<uint8_t, 5, 5, 1> > _repeat_edge__2_stencil_stream;
  // produce repeat_edge$2.stencil.stream
  {
   hls::stream<Stencil<uint8_t, 1, 1, 1> > _repeat_edge__2_stencil_update_stream;
   // produce repeat_edge$2.stencil_update.stream
   for (int _repeat_edge__2_scan_update__1 = 0; _repeat_edge__2_scan_update__1 < 0 + 264; _repeat_edge__2_scan_update__1++)
   {
    for (int _repeat_edge__2_scan_update__0 = 0; _repeat_edge__2_scan_update__0 < 0 + 264; _repeat_edge__2_scan_update__0++)
    {
     {
      Stencil<uint8_t, 1, 1, 1> _clamped_stream_stencil;
      // produce clamped_stream.stencil
      _clamped_stream_stencil = _clamped_stream_stencil_stream.read();
      (void)0;
      // consume clamped_stream.stencil
      {
       Stencil<uint8_t, 1, 1, 1> _repeat_edge__2_stencil_update;
       // produce repeat_edge$2.stencil_update
       uint8_t _1369 = _clamped_stream_stencil(0, 0, 0);
       _repeat_edge__2_stencil_update(0, 0, 0) = _1369;
       // consume repeat_edge$2.stencil_update
       _repeat_edge__2_stencil_update_stream.write(_repeat_edge__2_stencil_update);
       (void)0;
      } // realize _repeat_edge__2_stencil_update
     } // realize _clamped_stream_stencil
    } // for _repeat_edge__2_scan_update__0
   } // for _repeat_edge__2_scan_update__1
   // consume repeat_edge$2.stencil_update.stream
   linebuffer<264, 264, 1>(_repeat_edge__2_stencil_update_stream, _repeat_edge__2_stencil_stream);
   (void)0;
  } // realize _repeat_edge__2_stencil_update_stream
  // consume repeat_edge$2.stencil.stream
  {
   hls::stream<Stencil<uint8_t, 5, 5, 1> > _f2_stencil_stream;
   // produce f2.stencil.stream
   {
    hls::stream<Stencil<uint8_t, 1, 1, 1> > _f2_stencil_update_stream;
    // produce f2.stencil_update.stream
    for (int _f2_scan_update_y = 0; _f2_scan_update_y < 0 + 260; _f2_scan_update_y++)
    {
     for (int _f2_scan_update_x = 0; _f2_scan_update_x < 0 + 260; _f2_scan_update_x++)
     {
      {
       Stencil<uint8_t, 5, 5, 1> _repeat_edge__2_stencil;
       // produce repeat_edge$2.stencil
       _repeat_edge__2_stencil = _repeat_edge__2_stencil_stream.read();
       (void)0;
       // consume repeat_edge$2.stencil
       {
        Stencil<uint8_t, 1, 1, 1> _f2_stencil_update;
        // produce f2.stencil_update
        uint8_t _1370 = _repeat_edge__2_stencil(0, 0, 0);
        uint16_t _1371 = (uint16_t)(_1370);
        uint8_t _1372 = _repeat_edge__2_stencil(1, 0, 0);
        uint16_t _1373 = (uint16_t)(_1372);
        uint16_t _1374 = (uint16_t)(3);
        uint16_t _1375 = _1373 * _1374;
        uint16_t _1376 = _1371 + _1375;
        uint8_t _1377 = _repeat_edge__2_stencil(2, 0, 0);
        uint16_t _1378 = (uint16_t)(_1377);
        uint16_t _1379 = (uint16_t)(6);
        uint16_t _1380 = _1378 * _1379;
        uint16_t _1381 = _1376 + _1380;
        uint8_t _1382 = _repeat_edge__2_stencil(3, 0, 0);
        uint16_t _1383 = (uint16_t)(_1382);
        uint16_t _1384 = _1383 * _1374;
        uint16_t _1385 = _1381 + _1384;
        uint8_t _1386 = _repeat_edge__2_stencil(4, 0, 0);
        uint16_t _1387 = (uint16_t)(_1386);
        uint16_t _1388 = _1385 + _1387;
        uint8_t _1389 = _repeat_edge__2_stencil(0, 1, 0);
        uint16_t _1390 = (uint16_t)(_1389);
        uint16_t _1391 = _1390 * _1374;
        uint16_t _1392 = _1388 + _1391;
        uint8_t _1393 = _repeat_edge__2_stencil(1, 1, 0);
        uint16_t _1394 = (uint16_t)(_1393);
        uint16_t _1395 = (uint16_t)(15);
        uint16_t _1396 = _1394 * _1395;
        uint16_t _1397 = _1392 + _1396;
        uint8_t _1398 = _repeat_edge__2_stencil(2, 1, 0);
        uint16_t _1399 = (uint16_t)(_1398);
        uint16_t _1400 = (uint16_t)(25);
        uint16_t _1401 = _1399 * _1400;
        uint16_t _1402 = _1397 + _1401;
        uint8_t _1403 = _repeat_edge__2_stencil(3, 1, 0);
        uint16_t _1404 = (uint16_t)(_1403);
        uint16_t _1405 = _1404 * _1395;
        uint16_t _1406 = _1402 + _1405;
        uint8_t _1407 = _repeat_edge__2_stencil(4, 1, 0);
        uint16_t _1408 = (uint16_t)(_1407);
        uint16_t _1409 = _1408 * _1374;
        uint16_t _1410 = _1406 + _1409;
        uint8_t _1411 = _repeat_edge__2_stencil(0, 2, 0);
        uint16_t _1412 = (uint16_t)(_1411);
        uint16_t _1413 = _1412 * _1379;
        uint16_t _1414 = _1410 + _1413;
        uint8_t _1415 = _repeat_edge__2_stencil(1, 2, 0);
        uint16_t _1416 = (uint16_t)(_1415);
        uint16_t _1417 = _1416 * _1400;
        uint16_t _1418 = _1414 + _1417;
        uint8_t _1419 = _repeat_edge__2_stencil(2, 2, 0);
        uint16_t _1420 = (uint16_t)(_1419);
        uint16_t _1421 = (uint16_t)(44);
        uint16_t _1422 = _1420 * _1421;
        uint16_t _1423 = _1418 + _1422;
        uint8_t _1424 = _repeat_edge__2_stencil(3, 2, 0);
        uint16_t _1425 = (uint16_t)(_1424);
        uint16_t _1426 = _1425 * _1400;
        uint16_t _1427 = _1423 + _1426;
        uint8_t _1428 = _repeat_edge__2_stencil(4, 2, 0);
        uint16_t _1429 = (uint16_t)(_1428);
        uint16_t _1430 = _1429 * _1379;
        uint16_t _1431 = _1427 + _1430;
        uint8_t _1432 = _repeat_edge__2_stencil(0, 3, 0);
        uint16_t _1433 = (uint16_t)(_1432);
        uint16_t _1434 = _1433 * _1374;
        uint16_t _1435 = _1431 + _1434;
        uint8_t _1436 = _repeat_edge__2_stencil(1, 3, 0);
        uint16_t _1437 = (uint16_t)(_1436);
        uint16_t _1438 = _1437 * _1395;
        uint16_t _1439 = _1435 + _1438;
        uint8_t _1440 = _repeat_edge__2_stencil(2, 3, 0);
        uint16_t _1441 = (uint16_t)(_1440);
        uint16_t _1442 = _1441 * _1400;
        uint16_t _1443 = _1439 + _1442;
        uint8_t _1444 = _repeat_edge__2_stencil(3, 3, 0);
        uint16_t _1445 = (uint16_t)(_1444);
        uint16_t _1446 = _1445 * _1395;
        uint16_t _1447 = _1443 + _1446;
        uint8_t _1448 = _repeat_edge__2_stencil(4, 3, 0);
        uint16_t _1449 = (uint16_t)(_1448);
        uint16_t _1450 = _1449 * _1374;
        uint16_t _1451 = _1447 + _1450;
        uint8_t _1452 = _repeat_edge__2_stencil(0, 4, 0);
        uint16_t _1453 = (uint16_t)(_1452);
        uint16_t _1454 = _1451 + _1453;
        uint8_t _1455 = _repeat_edge__2_stencil(1, 4, 0);
        uint16_t _1456 = (uint16_t)(_1455);
        uint16_t _1457 = _1456 * _1374;
        uint16_t _1458 = _1454 + _1457;
        uint8_t _1459 = _repeat_edge__2_stencil(2, 4, 0);
        uint16_t _1460 = (uint16_t)(_1459);
        uint16_t _1461 = _1460 * _1379;
        uint16_t _1462 = _1458 + _1461;
        uint8_t _1463 = _repeat_edge__2_stencil(3, 4, 0);
        uint16_t _1464 = (uint16_t)(_1463);
        uint16_t _1465 = _1464 * _1374;
        uint16_t _1466 = _1462 + _1465;
        uint8_t _1467 = _repeat_edge__2_stencil(4, 4, 0);
        uint16_t _1468 = (uint16_t)(_1467);
        uint16_t _1469 = _1466 + _1468;
        uint16_t _1470 = _1469 >> 8;
        uint8_t _1471 = (uint8_t)(_1470);
        _f2_stencil_update(0, 0, 0) = _1471;
        // consume f2.stencil_update
        _f2_stencil_update_stream.write(_f2_stencil_update);
        (void)0;
       } // realize _f2_stencil_update
      } // realize _repeat_edge__2_stencil
     } // for _f2_scan_update_x
    } // for _f2_scan_update_y
    // consume f2.stencil_update.stream
    linebuffer<260, 260, 1>(_f2_stencil_update_stream, _f2_stencil_stream);
    (void)0;
   } // realize _f2_stencil_update_stream
   // consume f2.stencil.stream
   {
    hls::stream<Stencil<uint8_t, 1, 1, 1> > _f3_stream_stencil_update_stream;
    // produce f3_stream.stencil_update.stream
    for (int _f3_stream_scan_update_y = 0; _f3_stream_scan_update_y < 0 + 256; _f3_stream_scan_update_y++)
    {
     for (int _f3_stream_scan_update_x = 0; _f3_stream_scan_update_x < 0 + 256; _f3_stream_scan_update_x++)
     {
      {
       Stencil<uint8_t, 5, 5, 1> _f2_stencil;
       // produce f2.stencil
       _f2_stencil = _f2_stencil_stream.read();
       (void)0;
       // consume f2.stencil
       {
        Stencil<uint8_t, 1, 1, 1> _f3_stream_stencil_update;
        // produce f3_stream.stencil_update
        uint8_t _1472 = _f2_stencil(0, 0, 0);
        uint16_t _1473 = (uint16_t)(_1472);
        uint8_t _1474 = _f2_stencil(1, 0, 0);
        uint16_t _1475 = (uint16_t)(_1474);
        uint16_t _1476 = (uint16_t)(3);
        uint16_t _1477 = _1475 * _1476;
        uint16_t _1478 = _1473 + _1477;
        uint8_t _1479 = _f2_stencil(2, 0, 0);
        uint16_t _1480 = (uint16_t)(_1479);
        uint16_t _1481 = (uint16_t)(6);
        uint16_t _1482 = _1480 * _1481;
        uint16_t _1483 = _1478 + _1482;
        uint8_t _1484 = _f2_stencil(3, 0, 0);
        uint16_t _1485 = (uint16_t)(_1484);
        uint16_t _1486 = _1485 * _1476;
        uint16_t _1487 = _1483 + _1486;
        uint8_t _1488 = _f2_stencil(4, 0, 0);
        uint16_t _1489 = (uint16_t)(_1488);
        uint16_t _1490 = _1487 + _1489;
        uint8_t _1491 = _f2_stencil(0, 1, 0);
        uint16_t _1492 = (uint16_t)(_1491);
        uint16_t _1493 = _1492 * _1476;
        uint16_t _1494 = _1490 + _1493;
        uint8_t _1495 = _f2_stencil(1, 1, 0);
        uint16_t _1496 = (uint16_t)(_1495);
        uint16_t _1497 = (uint16_t)(15);
        uint16_t _1498 = _1496 * _1497;
        uint16_t _1499 = _1494 + _1498;
        uint8_t _1500 = _f2_stencil(2, 1, 0);
        uint16_t _1501 = (uint16_t)(_1500);
        uint16_t _1502 = (uint16_t)(25);
        uint16_t _1503 = _1501 * _1502;
        uint16_t _1504 = _1499 + _1503;
        uint8_t _1505 = _f2_stencil(3, 1, 0);
        uint16_t _1506 = (uint16_t)(_1505);
        uint16_t _1507 = _1506 * _1497;
        uint16_t _1508 = _1504 + _1507;
        uint8_t _1509 = _f2_stencil(4, 1, 0);
        uint16_t _1510 = (uint16_t)(_1509);
        uint16_t _1511 = _1510 * _1476;
        uint16_t _1512 = _1508 + _1511;
        uint8_t _1513 = _f2_stencil(0, 2, 0);
        uint16_t _1514 = (uint16_t)(_1513);
        uint16_t _1515 = _1514 * _1481;
        uint16_t _1516 = _1512 + _1515;
        uint8_t _1517 = _f2_stencil(1, 2, 0);
        uint16_t _1518 = (uint16_t)(_1517);
        uint16_t _1519 = _1518 * _1502;
        uint16_t _1520 = _1516 + _1519;
        uint8_t _1521 = _f2_stencil(2, 2, 0);
        uint16_t _1522 = (uint16_t)(_1521);
        uint16_t _1523 = (uint16_t)(44);
        uint16_t _1524 = _1522 * _1523;
        uint16_t _1525 = _1520 + _1524;
        uint8_t _1526 = _f2_stencil(3, 2, 0);
        uint16_t _1527 = (uint16_t)(_1526);
        uint16_t _1528 = _1527 * _1502;
        uint16_t _1529 = _1525 + _1528;
        uint8_t _1530 = _f2_stencil(4, 2, 0);
        uint16_t _1531 = (uint16_t)(_1530);
        uint16_t _1532 = _1531 * _1481;
        uint16_t _1533 = _1529 + _1532;
        uint8_t _1534 = _f2_stencil(0, 3, 0);
        uint16_t _1535 = (uint16_t)(_1534);
        uint16_t _1536 = _1535 * _1476;
        uint16_t _1537 = _1533 + _1536;
        uint8_t _1538 = _f2_stencil(1, 3, 0);
        uint16_t _1539 = (uint16_t)(_1538);
        uint16_t _1540 = _1539 * _1497;
        uint16_t _1541 = _1537 + _1540;
        uint8_t _1542 = _f2_stencil(2, 3, 0);
        uint16_t _1543 = (uint16_t)(_1542);
        uint16_t _1544 = _1543 * _1502;
        uint16_t _1545 = _1541 + _1544;
        uint8_t _1546 = _f2_stencil(3, 3, 0);
        uint16_t _1547 = (uint16_t)(_1546);
        uint16_t _1548 = _1547 * _1497;
        uint16_t _1549 = _1545 + _1548;
        uint8_t _1550 = _f2_stencil(4, 3, 0);
        uint16_t _1551 = (uint16_t)(_1550);
        uint16_t _1552 = _1551 * _1476;
        uint16_t _1553 = _1549 + _1552;
        uint8_t _1554 = _f2_stencil(0, 4, 0);
        uint16_t _1555 = (uint16_t)(_1554);
        uint16_t _1556 = _1553 + _1555;
        uint8_t _1557 = _f2_stencil(1, 4, 0);
        uint16_t _1558 = (uint16_t)(_1557);
        uint16_t _1559 = _1558 * _1476;
        uint16_t _1560 = _1556 + _1559;
        uint8_t _1561 = _f2_stencil(2, 4, 0);
        uint16_t _1562 = (uint16_t)(_1561);
        uint16_t _1563 = _1562 * _1481;
        uint16_t _1564 = _1560 + _1563;
        uint8_t _1565 = _f2_stencil(3, 4, 0);
        uint16_t _1566 = (uint16_t)(_1565);
        uint16_t _1567 = _1566 * _1476;
        uint16_t _1568 = _1564 + _1567;
        uint8_t _1569 = _f2_stencil(4, 4, 0);
        uint16_t _1570 = (uint16_t)(_1569);
        uint16_t _1571 = _1568 + _1570;
        uint16_t _1572 = _1571 >> 8;
        uint8_t _1573 = (uint8_t)(_1572);
        _f3_stream_stencil_update(0, 0, 0) = _1573;
        // consume f3_stream.stencil_update
        _f3_stream_stencil_update_stream.write(_f3_stream_stencil_update);
        (void)0;
       } // realize _f3_stream_stencil_update
      } // realize _f2_stencil
     } // for _f3_stream_scan_update_x
    } // for _f3_stream_scan_update_y
    // consume f3_stream.stencil_update.stream
    linebuffer<256, 256, 1>(_f3_stream_stencil_update_stream, _f3_stream_stencil_stream);
    (void)0;
   } // realize _f3_stream_stencil_update_stream
  } // realize _f2_stencil_stream
 } // realize _repeat_edge__2_stencil_stream
} // kernel hls_target_f3_stream_stencil_stream



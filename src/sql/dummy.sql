-- =============================================================================
-- dummy.sql — Sample data for manual development/testing.
-- Safe to run multiple times; INSERT OR IGNORE prevents duplicates.
-- =============================================================================

PRAGMA foreign_keys = ON;

-- -----------------------------------------------------------------------------
-- Storage locations  (hierarchical)
--   All (0, virtual root)
--   ├─ Cabinet A  (11) ──  Drawer A1, A2, A3
--   ├─ Cabinet B  (12) ──  Drawer B1, B2
--   ├─ Shelf C    (13) ──  Shelf C1, C2
--   ├─ Reel Boxes (14) ──  Reel Box 1, Reel Box 2
--   ├─ Loose Bin  (10)
--   ├─ Workbench  (15) ──  Tray W1, Tray W2  [unused — for grayed-out node test]
--   └─ Archive    (18)     [unused — for grayed-out node test]
-- Parts reference leaf ids 1-10; parent container nodes use ids 11-14.
-- id=0 'All' is seeded by init.sql, not repeated here.
-- -----------------------------------------------------------------------------
INSERT OR IGNORE INTO storage_locations (id, name, parent_id) VALUES
    (11, 'Cabinet A',   0),
    (12, 'Cabinet B',   0),
    (13, 'Shelf C',     0),
    (14, 'Reel Boxes',  0),
    ( 1, 'Drawer A1',  11),
    ( 2, 'Drawer A2',  11),
    ( 3, 'Drawer A3',  11),
    ( 4, 'Drawer B1',  12),
    ( 5, 'Drawer B2',  12),
    ( 6, 'Shelf C1',   13),
    ( 7, 'Shelf C2',   13),
    ( 8, 'Reel Box 1', 14),
    ( 9, 'Reel Box 2', 14),
    (10, 'Loose Bin',   0),
    -- Unused locations (no parts assigned) — verify grayed-out rendering
    (15, 'Workbench',   0),
    (16, 'Tray W1',    15),
    (17, 'Tray W2',    15),
    (18, 'Archive',     0);

-- -----------------------------------------------------------------------------
-- Parts
-- category_id references (from init.sql):
--   22  Chip Resistors (SMD)        25  Current Sense Resistors
--   23  Through-Hole Resistors      26  High-Power Resistors
--   27  Ceramic Capacitors (MLCC)   28  Electrolytic Capacitors
--   29  Tantalum Capacitors         33  SMD Power Inductors
--   50  Rectifier Diodes            51  Schottky Diodes
--   52  Zener Diodes                53  TVS Diodes
--   55  Bipolar (BJT)               40  MOSFETs
--   60  Op-Amps                     66  LDO Regulators
--   67  DC-DC (Buck)                44  Microcontrollers (MCU)
--   58  Logic Gates & Buffers       59  Timers
--   81  Pin Headers & Sockets       82  USB Connectors
--   88  Terminal Blocks             90  Fuses
--   91  Resettable Fuses (PTC)      95  LEDs
--   97  Optocouplers               101  Temperature Sensors
--  104  Accelerometers & Gyroscopes
-- -----------------------------------------------------------------------------
INSERT OR IGNORE INTO parts (id, name, quantity, storage_location_id, category_id) VALUES
    -- ── Chip Resistors (SMD) ──────────────────────────────────────────────
    (  1, 'Resistor 100R 0402 1% 100mW',            500,  8, 22),
    (  2, 'Resistor 1k 0402 1% 100mW',              480,  8, 22),
    (  3, 'Resistor 10k 0402 1% 100mW',             450,  8, 22),
    (  4, 'Resistor 100k 0402 1% 100mW',            300,  8, 22),
    (  5, 'Resistor 4.7k 0603 5% 100mW',            200,  8, 22),
    (  6, 'Resistor 0R 0603 (jumper)',               100,  8, 22),
    -- ── Through-Hole Resistors ───────────────────────────────────────────
    (  7, 'Resistor 220R 1/4W 5% axial',             80,  1, 23),
    (  8, 'Resistor 1k 1/4W 5% axial',               75,  1, 23),
    (  9, 'Resistor 10k 1/4W 5% axial',              60,  1, 23),
    ( 10, 'Resistor 470R 1/2W 5% axial',             40,  1, 23),
    -- ── Current Sense Resistors ──────────────────────────────────────────
    ( 11, 'Shunt 10mR 1W 1% 2512',                   30,  9, 25),
    ( 12, 'Shunt 100mR 0.5W 1% 1206',                25,  9, 25),
    -- ── Ceramic Capacitors (MLCC) ────────────────────────────────────────
    ( 13, 'Cap 100nF 0402 X7R 50V',                 600,  8, 27),
    ( 14, 'Cap 1uF 0402 X5R 10V',                   400,  8, 27),
    ( 15, 'Cap 10nF 0402 C0G 50V',                  250,  8, 27),
    ( 16, 'Cap 4.7uF 0805 X5R 16V',                 150,  8, 27),
    ( 17, 'Cap 10uF 0805 X5R 10V',                  120,  8, 27),
    -- ── Electrolytic Capacitors ──────────────────────────────────────────
    ( 18, 'Cap Elyt 10uF 25V 5mm',                   60,  2, 28),
    ( 19, 'Cap Elyt 100uF 16V 6.3mm',                50,  2, 28),
    ( 20, 'Cap Elyt 470uF 10V 8mm',                  30,  2, 28),
    ( 21, 'Cap Elyt 1000uF 6.3V 10mm',               20,  2, 28),
    -- ── Tantalum Capacitors ──────────────────────────────────────────────
    ( 22, 'Cap Tant 10uF 10V B-case',                40,  9, 29),
    ( 23, 'Cap Tant 100uF 6.3V D-case',              15,  9, 29),
    -- ── SMD Power Inductors ──────────────────────────────────────────────
    ( 24, 'Inductor 4.7uH 1.5A 5432',                50,  9, 33),
    ( 25, 'Inductor 10uH 1A 4020',                   40,  9, 33),
    ( 26, 'Inductor 22uH 0.8A 4020',                 30,  9, 33),
    -- ── Rectifier Diodes ─────────────────────────────────────────────────
    ( 27, '1N4007 1A 1000V DO-41',                   80,  3, 50),
    ( 28, '1N4148 100mA 100V DO-35',                120,  3, 50),
    -- ── Schottky Diodes ──────────────────────────────────────────────────
    ( 29, 'SS14 1A 40V SMA',                         60,  3, 51),
    ( 30, 'BAT54S dual 200mA 30V SOT-23',            50,  3, 51),
    ( 31, 'MBRS340 3A 40V SMC',                      25,  3, 51),
    -- ── Zener Diodes ─────────────────────────────────────────────────────
    ( 32, 'BZX84-C5V1 5.1V 300mW SOT-23',            80,  3, 52),
    ( 33, 'BZX84-C3V3 3.3V 300mW SOT-23',            70,  3, 52),
    -- ── TVS Diodes ───────────────────────────────────────────────────────
    ( 34, 'SMBJ5.0A 5V uni 600W SMB',                40,  3, 53),
    ( 35, 'PRTR5V0U2X dual-rail 5V SOT-363',         35,  3, 53),
    -- ── BJT Transistors ──────────────────────────────────────────────────
    ( 36, 'BC817-40 NPN 45V 500mA SOT-23',           80,  4, 55),
    ( 37, 'BC807-40 PNP 45V 500mA SOT-23',           70,  4, 55),
    ( 38, '2N2222A NPN 40V 600mA TO-92',             50,  4, 55),
    -- ── MOSFETs ──────────────────────────────────────────────────────────
    ( 39, 'SI2302 N-ch 20V 2.7A SOT-23',             60,  4, 40),
    ( 40, 'AO3401 P-ch 30V 4A SOT-23',               55,  4, 40),
    ( 41, 'IRLZ44N N-ch 55V 47A TO-220',             15,  6, 40),
    -- ── Op-Amps ──────────────────────────────────────────────────────────
    ( 42, 'MCP6001 1MHz rail-to-rail SOT-23-5',      30,  5, 60),
    ( 43, 'LM358 dual 1MHz DIP-8',                   20,  5, 60),
    ( 44, 'TL071 JFET 3MHz DIP-8',                   15,  5, 60),
    ( 45, 'OPA2134 dual audio SOP-8',                10,  5, 60),
    -- ── LDO Regulators ───────────────────────────────────────────────────
    ( 46, 'AMS1117-3.3 1A SOT-223',                  40,  5, 66),
    ( 47, 'AMS1117-5.0 1A SOT-223',                  35,  5, 66),
    ( 48, 'MIC5219-3.3 500mA SOT-23-5',              25,  5, 66),
    ( 49, 'TPS7A4901 ADJ 1A D2PAK',                  10,  5, 66),
    -- ── DC-DC Buck Converters ─────────────────────────────────────────────
    ( 50, 'MP2307 3A 23V SOP-8',                     20,  5, 67),
    ( 51, 'TPS54331 3A 28V SOP-8',                   15,  5, 67),
    ( 52, 'LM2596-5 3A TO-263-5',                    10,  6, 67),
    -- ── Microcontrollers ─────────────────────────────────────────────────
    ( 53, 'ATmega328P-AU 20MHz TQFP-32',             12,  7, 44),
    ( 54, 'STM32F103C8T6 72MHz LQFP-48',             10,  7, 44),
    ( 55, 'RP2040 133MHz QFN-56',                     8,  7, 44),
    ( 56, 'ESP32-D0WDQ6 240MHz QFN-48',               6,  7, 44),
    ( 57, 'ATtiny85-20PU 20MHz DIP-8',                0,  7, 44),
    -- ── Logic Gates & Buffers ────────────────────────────────────────────
    ( 58, '74HC00 quad NAND SOIC-14',                30,  5, 58),
    ( 59, '74HC04 hex inverter SOIC-14',              0,  5, 58),
    ( 60, '74HC245 octal bus SOIC-20',               20,  5, 58),
    -- ── Timers ───────────────────────────────────────────────────────────
    ( 61, 'NE555P timer DIP-8',                      40,  5, 59),
    ( 62, 'NE556N dual timer DIP-14',                 0,  5, 59),
    -- ── Pin Headers & Sockets ────────────────────────────────────────────
    ( 63, 'Pin header 2.54mm 1x40 breakable M',      25, 10, 81),
    ( 64, 'Pin header 2.54mm 2x20 M',                 0, 10, 81),
    ( 65, 'Socket 2.54mm 1x8 F',                     30, 10, 81),
    ( 66, 'Socket 2.54mm 2x3 F (ISP)',               20, 10, 81),
    -- ── USB Connectors ───────────────────────────────────────────────────
    ( 67, 'USB-B micro SMD',                         20,  2, 82),
    ( 68, 'USB-C 16-pin SMD',                        15,  2, 82),
    ( 69, 'USB-A THT horizontal',                    10,  2, 82),
    -- ── Terminal Blocks ──────────────────────────────────────────────────
    ( 70, 'Terminal block 5.08mm 2-pin THT',         30,  2, 88),
    ( 71, 'Terminal block 5.08mm 3-pin THT',         20,  2, 88),
    ( 72, 'Terminal block 3.5mm 4-pin SMD',          15,  2, 88),
    -- ── Fuses ────────────────────────────────────────────────────────────
    ( 73, 'Fuse 500mA 250V 5x20mm slow',             20, 10, 90),
    ( 74, 'Fuse 1A 250V 5x20mm fast',                20, 10, 90),
    ( 75, 'Fuse 3A 32V ATO blade',                   15, 10, 90),
    -- ── Resettable Fuses (PTC) ───────────────────────────────────────────
    ( 76, 'PTC 100mA 60V 0805',                      40,  9, 91),
    ( 77, 'PTC 500mA 16V 1812',                      25,  9, 91),
    -- ── LEDs ─────────────────────────────────────────────────────────────
    ( 78, 'LED red 0805 2V 20mA',                   100,  8, 95),
    ( 79, 'LED green 0805 3.2V 20mA',                80,  8, 95),
    ( 80, 'LED blue 0805 3.4V 20mA',                 60,  8, 95),
    ( 81, 'LED white 0805 3.2V 20mA',                60,  8, 95),
    ( 82, 'LED yellow 3mm THT 2V 20mA',              50,  1, 95),
    -- ── Optocouplers ─────────────────────────────────────────────────────
    ( 83, 'PC817 1-ch 50V CTR 100% DIP-4',           30,  4, 97),
    ( 84, 'TLP291 1-ch 3.75kV SOP-4',                20,  4, 97),
    -- ── Temperature Sensors ──────────────────────────────────────────────
    ( 85, 'DS18B20 1-Wire TO-92',                    10,  4, 101),
    ( 86, 'LM35DZ 10mV/C TO-92',                     8,  4, 101),
    ( 87, 'NTC 10k 1% 0402',                         50,  9, 101),
    -- ── Accelerometers & Gyroscopes ──────────────────────────────────────
    ( 88, 'MPU-6050 6-DoF I2C QFN-24',               6,  7, 104),
    ( 89, 'LIS3DH 3-axis SPI/I2C LGA-16',            5,  7, 104);

-- -----------------------------------------------------------------------------
-- Sample files (paths are illustrative; no real files required)
-- -----------------------------------------------------------------------------
INSERT OR IGNORE INTO files (id, path, type, description) VALUES
    (1, 'datasheets/ds18b20.pdf',      'datasheet', 'DS18B20 datasheet'),
    (2, 'datasheets/stm32f103c8.pdf',  'datasheet', 'STM32F103C8 reference manual'),
    (3, 'datasheets/mpu6050.pdf',      'datasheet', 'MPU-6050 product spec'),
    (4, 'datasheets/lm2596.pdf',       'datasheet', 'LM2596 datasheet'),
    (5, 'models/stm32f103c8t6.step',   'model',     'STM32F103C8T6 STEP model'),
    (6, 'models/mpu6050.step',         'model',     'MPU-6050 STEP model');

-- -----------------------------------------------------------------------------
-- File associations
-- -----------------------------------------------------------------------------
INSERT OR IGNORE INTO part_files (part_id, file_id) VALUES
    (85, 1),   -- DS18B20                → ds18b20.pdf
    (54, 2),   -- STM32F103C8T6          → stm32f103c8.pdf
    (54, 5),   -- STM32F103C8T6          → .step model
    (88, 3),   -- MPU-6050               → mpu6050.pdf
    (88, 6),   -- MPU-6050               → .step model
    (52, 4);   -- LM2596-5               → lm2596.pdf

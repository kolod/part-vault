-- Enable foreign key constraint enforcement (required explicitly in SQLite).
PRAGMA foreign_keys = ON;

-- Hierarchical categories for organising parts (e.g. Passive → Capacitors → Electrolytic).
-- parent_id is NULL for top-level categories; child categories reference their parent.
-- Deleting a parent sets children's parent_id to NULL (they become top-level).
CREATE TABLE IF NOT EXISTS categories (
    id                      INTEGER PRIMARY KEY,
    name                    TEXT    NOT NULL,
    parent_id               INTEGER REFERENCES categories(id) ON DELETE SET NULL
);

-- Physical storage locations where parts are kept (e.g. "Shelf A3", "Drawer 12").
CREATE TABLE IF NOT EXISTS storage_locations (
    id                      INTEGER PRIMARY KEY,
    name                    TEXT NOT NULL
);

-- Core parts table.
-- Each part belongs to one optional category and one optional storage location.
-- Deleting a category or location sets the corresponding FK to NULL rather than
-- removing the part.
CREATE TABLE IF NOT EXISTS parts (
    id                      INTEGER PRIMARY KEY,
    name                    TEXT    NOT NULL,
    quantity                INTEGER NOT NULL DEFAULT 0,
    storage_location_id     INTEGER REFERENCES storage_locations(id) ON DELETE SET NULL,
    category_id             INTEGER REFERENCES categories(id)        ON DELETE SET NULL
);

-- Files stored on the local drive and associated with parts.
-- type constrains the kind of file:
--   'datasheet' - PDF or similar component documentation
--   'cad'       - schematic / PCB design file
--   'model'     - 3-D model (STEP, STL, ...)
-- path must be unique so the same physical file is never duplicated in the catalogue.
CREATE TABLE IF NOT EXISTS files (
    id                      INTEGER PRIMARY KEY,
    path                    TEXT NOT NULL UNIQUE,
    type                    TEXT NOT NULL CHECK(type IN ('datasheet', 'cad', 'model')),
    description             TEXT
);

-- Many-to-many join between parts and files.
-- A part can have multiple files; a file can be shared by multiple parts.
-- Deleting either a part or a file automatically removes the corresponding rows here.
CREATE TABLE IF NOT EXISTS part_files (
    part_id                 INTEGER NOT NULL REFERENCES parts(id) ON DELETE CASCADE,
    file_id                 INTEGER NOT NULL REFERENCES files(id) ON DELETE CASCADE,
    PRIMARY KEY (part_id, file_id)
);

-- ---------------------------------------------------------------------------
-- Seed data: default component categories (mirrors Mouser top-level taxonomy).
-- INSERT OR IGNORE ensures re-running init never duplicates rows.
-- ---------------------------------------------------------------------------

INSERT OR IGNORE INTO categories (id, name, parent_id) VALUES
    -- Top-level categories (parent_id IS NULL)
    (  1, 'Passive Components',               NULL),
    (  2, 'Semiconductors',                   NULL),
    (  3, 'Electromechanical',                NULL),
    (  4, 'Connectors',                       NULL),
    (  5, 'RF / Wireless',                    NULL),
    (  6, 'Power',                            NULL),
    (  7, 'Optoelectronics',                  NULL),
    (  8, 'Sensors & Transducers',            NULL),
    (  9, 'Embedded Computers & SBCs',        NULL),
    ( 10, 'Test & Measurement',               NULL),
    ( 11, 'Circuit Protection',               NULL),
    ( 12, 'Cables & Wires',                   NULL),
    ( 13, 'Tools & Supplies',                 NULL);
    -- Passive Components (parent 1)
    (101, 'Resistors',                         1),
    (102, 'Capacitors',                        1),
    (103, 'Inductors & Chokes',                1),
    (104, 'Transformers',                      1),
    (105, 'Crystals & Oscillators',            1),
    (106, 'Filters',                           1),
    (107, 'Potentiometers & Trimmers',         1),
    (108, 'Ferrite Beads',                     1);
    -- Resistors (parent 101)
    (1011, 'Chip Resistors (SMD)',             101),
    (1012, 'Through-Hole Resistors',           101),
    (1013, 'Resistor Networks & Arrays',       101),
    (1014, 'Current Sense Resistors',          101),
    (1015, 'High-Power Resistors',             101);
    -- Capacitors (parent 102)
    (1021, 'Ceramic Capacitors (MLCC)',        102),
    (1022, 'Electrolytic Capacitors',          102),
    (1023, 'Tantalum Capacitors',              102),
    (1024, 'Film Capacitors',                  102),
    (1025, 'Supercapacitors / Ultracapacitors',102),
    (1026, 'Mica & PTFE Capacitors',          102);
    -- Inductors & Chokes (parent 103)
    (1031, 'SMD Power Inductors',              103),
    (1032, 'Through-Hole Inductors',           103),
    (1033, 'Common Mode Chokes',               103),
    (1034, 'Coupled Inductors',                103);
    -- Semiconductors (parent 2)
    (201, 'Diodes',                            2),
    (202, 'Transistors',                       2),
    (203, 'Integrated Circuits',               2),
    (204, 'MOSFETs',                           2),
    (205, 'IGBTs',                             2),
    (206, 'Thyristors & SCRs',                 2),
    (207, 'Memory ICs',                        2),
    (208, 'Microcontrollers (MCU)',             2),
    (209, 'Microprocessors (MPU)',              2),
    (210, 'FPGAs & CPLDs',                     2),
    (211, 'Interface ICs',                     2),
    (212, 'Analog ICs',                        2),
    (213, 'Power Management ICs',              2);
    -- Diodes (parent 201)
    (2011, 'Rectifier Diodes',                 201),
    (2012, 'Schottky Diodes',                  201),
    (2013, 'Zener Diodes',                     201),
    (2014, 'TVS Diodes',                       201),
    (2015, 'Switching Diodes',                 201);
    -- Transistors (parent 202)
    (2021, 'Bipolar (BJT)',                    202),
    (2022, 'JFET',                             202),
    (2023, 'Darlington Transistors',           202);
    -- Integrated Circuits (parent 203)
    (2031, 'Logic Gates & Buffers',            203),
    (2032, 'Timers',                           203),
    (2033, 'Operational Amplifiers (Op-Amp)',  203),
    (2034, 'Comparators',                      203),
    (2035, 'Multiplexers & Switches',          203),
    (2036, 'Clock & Timing ICs',               203),
    (2037, 'Audio Amplifiers',                 203),
    (2038, 'RF Amplifiers',                    203);
    -- Power Management ICs (parent 213)
    (2131, 'LDO Regulators',                   213),
    (2132, 'DC-DC Converters (Buck)',          213),
    (2133, 'DC-DC Converters (Boost)',         213),
    (2134, 'DC-DC Converters (Buck-Boost)',    213),
    (2135, 'Voltage References',               213),
    (2136, 'Battery Management ICs',           213),
    (2137, 'Gate Drivers',                     213),
    (2138, 'Hot-Swap & eFuse Controllers',     213);
    -- Electromechanical (parent 3)
    (301, 'Relays',                            3),
    (302, 'Switches',                          3),
    (303, 'Pushbuttons',                       3),
    (304, 'Encoders',                          3),
    (305, 'Motors',                            3),
    (306, 'Fans & Blowers',                    3),
    (307, 'Solenoids',                         3);
    -- Connectors (parent 4)
    (401, 'Pin Headers & Sockets',             4),
    (402, 'USB Connectors',                    4),
    (403, 'D-Sub Connectors',                  4),
    (404, 'Board-to-Board Connectors',         4),
    (405, 'FFC / FPC Connectors',              4),
    (406, 'Power Connectors',                  4),
    (407, 'RF Coaxial Connectors',             4),
    (408, 'Terminal Blocks',                   4),
    (409, 'Circular Connectors',               4);
    -- Circuit Protection (parent 11)
    (1101, 'Fuses',                            11),
    (1102, 'Resettable Fuses (PTC)',           11),
    (1103, 'ESD Protection Arrays',            11),
    (1104, 'Varistors (MOV)',                  11),
    (1105, 'Gas Discharge Tubes',              11);
    -- Optoelectronics (parent 7)
    (701, 'LEDs',                              7),
    (702, 'LED Drivers',                       7),
    (703, 'Optocouplers / Optoisolators',      7),
    (704, 'Photodiodes & Phototransistors',    7),
    (705, 'Displays',                          7),
    (706, 'Laser Diodes',                      7);
    -- Sensors & Transducers (parent 8)
    (801, 'Temperature Sensors',               8),
    (802, 'Humidity Sensors',                  8),
    (803, 'Pressure Sensors',                  8),
    (804, 'Accelerometers & Gyroscopes',       8),
    (805, 'Current & Power Sensors',           8),
    (806, 'Magnetic & Hall Effect Sensors',    8),
    (807, 'Proximity & Distance Sensors',      8),
    (808, 'Gas & Chemical Sensors',            8);

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
-- Hierarchical: a location can have sub-locations (e.g. Cabinet → Drawer → Compartment).
-- parent_id is NULL for top-level locations; deleting a parent sets children's parent_id to NULL.
CREATE TABLE IF NOT EXISTS storage_locations (
    id                      INTEGER PRIMARY KEY,
    name                    TEXT NOT NULL,
    parent_id               INTEGER REFERENCES storage_locations(id) ON DELETE SET NULL
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
    category_id             INTEGER REFERENCES categories(id)        ON DELETE SET NULL,
    last_change             TEXT    NOT NULL DEFAULT (strftime('%Y-%m-%d %H:%M:%S', 'now', 'localtime'))
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

-- Seed data: default component categories (mirrors Mouser top-level taxonomy).
-- INSERT OR IGNORE ensures re-running init never duplicates rows.
INSERT OR IGNORE INTO storage_locations (id, name, parent_id) VALUES
    -- Virtual root — allows tree views to show every location under one node.
    (0, 'All', NULL);

INSERT OR IGNORE INTO categories (id, name, parent_id) VALUES
    -- Virtual root — allows tree views to show every category under one node.
    (  0, 'All',                            NULL),
    -- Top-level categories (parent_id = 0)
    (  1, 'Passive Components',                0),
    (  2, 'Semiconductors',                    0),
    (  3, 'Electromechanical',                 0),
    (  4, 'Connectors',                        0),
    (  5, 'RF / Wireless',                     0),
    (  6, 'Power',                             0),
    (  7, 'Optoelectronics',                   0),
    (  8, 'Sensors & Transducers',             0),
    (  9, 'Embedded Computers & SBCs',         0),
    ( 10, 'Test & Measurement',                0),
    ( 11, 'Circuit Protection',                0),
    ( 12, 'Cables & Wires',                    0),
    ( 13, 'Tools & Supplies',                  0),
    -- Passive Components (parent 1)
    ( 14, 'Resistors',                         1),
    ( 15, 'Capacitors',                        1),
    ( 16, 'Inductors & Chokes',                1),
    ( 17, 'Transformers',                      1),
    ( 18, 'Crystals & Oscillators',            1),
    ( 19, 'Filters',                           1),
    ( 20, 'Potentiometers & Trimmers',         1),
    ( 21, 'Ferrite Beads',                     1),
    -- Resistors (parent 14)
    ( 22, 'Chip Resistors (SMD)',             14),
    ( 23, 'Through-Hole Resistors',           14),
    ( 24, 'Resistor Networks & Arrays',       14),
    ( 25, 'Current Sense Resistors',          14),
    ( 26, 'High-Power Resistors',             14),
    -- Capacitors (parent 15)
    ( 27, 'Ceramic Capacitors (MLCC)',        15),
    ( 28, 'Electrolytic Capacitors',          15),
    ( 29, 'Tantalum Capacitors',              15),
    ( 30, 'Film Capacitors',                  15),
    ( 31, 'Supercapacitors / Ultracapacitors',15),
    ( 32, 'Mica & PTFE Capacitors',           15),
    -- Inductors & Chokes (parent 16)
    ( 33, 'SMD Power Inductors',              16),
    ( 34, 'Through-Hole Inductors',           16),
    ( 35, 'Common Mode Chokes',               16),
    ( 36, 'Coupled Inductors',                16),
    -- Semiconductors (parent 2)
    ( 37, 'Diodes',                            2),
    ( 38, 'Transistors',                       2),
    ( 39, 'Integrated Circuits',               2),
    ( 40, 'MOSFETs',                           2),
    ( 41, 'IGBTs',                             2),
    ( 42, 'Thyristors & SCRs',                 2),
    ( 43, 'Memory ICs',                        2),
    ( 44, 'Microcontrollers (MCU)',            2),
    ( 45, 'Microprocessors (MPU)',             2),
    ( 46, 'FPGAs & CPLDs',                     2),
    ( 47, 'Interface ICs',                     2),
    ( 48, 'Analog ICs',                        2),
    ( 49, 'Power Management ICs',              2),
    -- Diodes (parent 37)
    ( 50, 'Rectifier Diodes',                 37),
    ( 51, 'Schottky Diodes',                  37),
    ( 52, 'Zener Diodes',                     37),
    ( 53, 'TVS Diodes',                       37),
    ( 54, 'Switching Diodes',                 37),
    -- Transistors (parent 38)
    ( 55, 'Bipolar (BJT)',                    38),
    ( 56, 'JFET',                             38),
    ( 57, 'Darlington Transistors',           38),
    -- Integrated Circuits (parent 39)
    ( 58, 'Logic Gates & Buffers',            39),
    ( 59, 'Timers',                           39),
    ( 60, 'Operational Amplifiers (Op-Amp)',  39),
    ( 61, 'Comparators',                      39),
    ( 62, 'Multiplexers & Switches',          39),
    ( 63, 'Clock & Timing ICs',               39),
    ( 64, 'Audio Amplifiers',                 39),
    ( 65, 'RF Amplifiers',                    39),
    -- Power Management ICs (parent 49)
    ( 66, 'LDO Regulators',                   49),
    ( 67, 'DC-DC Converters (Buck)',          49),
    ( 68, 'DC-DC Converters (Boost)',         49),
    ( 69, 'DC-DC Converters (Buck-Boost)',    49),
    ( 70, 'Voltage References',               49),
    ( 71, 'Battery Management ICs',           49),
    ( 72, 'Gate Drivers',                     49),
    ( 73, 'Hot-Swap & eFuse Controllers',     49),
    -- Electromechanical (parent 3)
    ( 74, 'Relays',                            3),
    ( 75, 'Switches',                          3),
    ( 76, 'Pushbuttons',                       3),
    ( 77, 'Encoders',                          3),
    ( 78, 'Motors',                            3),
    ( 79, 'Fans & Blowers',                    3),
    ( 80, 'Solenoids',                         3),
    -- Connectors (parent 4)
    ( 81, 'Pin Headers & Sockets',             4),
    ( 82, 'USB Connectors',                    4),
    ( 83, 'D-Sub Connectors',                  4),
    ( 84, 'Board-to-Board Connectors',         4),
    ( 85, 'FFC / FPC Connectors',              4),
    ( 86, 'Power Connectors',                  4),
    ( 87, 'RF Coaxial Connectors',             4),
    ( 88, 'Terminal Blocks',                   4),
    ( 89, 'Circular Connectors',               4),
    -- Circuit Protection (parent 11)
    ( 90, 'Fuses',                            11),
    ( 91, 'Resettable Fuses (PTC)',           11),
    ( 92, 'ESD Protection Arrays',            11),
    ( 93, 'Varistors (MOV)',                  11),
    ( 94, 'Gas Discharge Tubes',              11),
    -- Optoelectronics (parent 7)
    ( 95, 'LEDs',                              7),
    ( 96, 'LED Drivers',                       7),
    ( 97, 'Optocouplers / Optoisolators',      7),
    ( 98, 'Photodiodes & Phototransistors',    7),
    ( 99, 'Displays',                          7),
    (100, 'Laser Diodes',                      7),
    -- Sensors & Transducers (parent 8)
    (101, 'Temperature Sensors',               8),
    (102, 'Humidity Sensors',                  8),
    (103, 'Pressure Sensors',                  8),
    (104, 'Accelerometers & Gyroscopes',       8),
    (105, 'Current & Power Sensors',           8),
    (106, 'Magnetic & Hall Effect Sensors',    8),
    (107, 'Proximity & Distance Sensors',      8),
    (108, 'Gas & Chemical Sensors',            8);

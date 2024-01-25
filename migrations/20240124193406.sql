/**
 * =======================================================================
 * PostgreSQL 16
 * =======================================================================
 */

-- ❯ sudo -u postgres psql -f init.sql
DO $$ 
BEGIN
  IF NOT EXISTS (SELECT FROM pg_user WHERE usename = 'andlifecare_app') THEN
    CREATE USER andlifecare_app;
  END IF;
END $$;

/**
 * After creating the andlifecare_app user, need to:
 * 
 * 1. ❯ sudo -u postgres psql
 * 2. postgres=# ALTER USER andlifecare_app WITH PASSWORD 'password';
 * 3. postgres=# \q
 * 4. ❯ sudo service postgresql restart
 * 5. ❯ psql -U andlifecare_app -h localhost -d andlifecare -W
 * 6. andlifecare=> ...
 * 
 * We don't parameterize the andlifecate_app user password in this script
 * because command-line arguments can be visible in process lists or logs
 */

-- `CREATE DATABASE` cannot be executed inside a transaction block, so we use `\gexec` instead.
SELECT 'CREATE DATABASE andlifecare OWNER andlifecare_app' WHERE NOT EXISTS (SELECT FROM pg_database WHERE datname = 'andlifecare')\gexec

\connect andlifecare;
CREATE SCHEMA IF NOT EXISTS app AUTHORIZATION andlifecare_app;

-- Let's make sure no one uses the public schema.
REVOKE ALL PRIVILEGES ON SCHEMA public FROM PUBLIC;

-- to use uuid_generate_v4() function
CREATE EXTENSION IF NOT EXISTS "uuid-ossp";

CREATE TABLE IF NOT EXISTS app.countries (
    id SERIAL PRIMARY KEY,
    iso CHAR(2) NOT NULL UNIQUE,
    name VARCHAR(80) NOT NULL,
    nicename VARCHAR(80) NOT NULL,
    iso3 CHAR(3) DEFAULT NULL,
    numcode SMALLINT DEFAULT NULL,
    created_at TIMESTAMP WITH TIME ZONE NOT NULL DEFAULT NOW(),
    updated_at TIMESTAMP WITH TIME ZONE NOT NULL DEFAULT NOW()
);

INSERT INTO app.countries (id, iso, name, nicename, iso3, numcode) VALUES
    (1, 'AF', 'AFGHANISTAN', 'Afghanistan', 'AFG', 4),
    (2, 'AL', 'ALBANIA', 'Albania', 'ALB', 8),
    (3, 'DZ', 'ALGERIA', 'Algeria', 'DZA', 12),
    (4, 'AS', 'AMERICAN SAMOA', 'American Samoa', 'ASM', 16),
    (5, 'AD', 'ANDORRA', 'Andorra', 'AND', 20),
    (6, 'AO', 'ANGOLA', 'Angola', 'AGO', 24),
    (7, 'AI', 'ANGUILLA', 'Anguilla', 'AIA', 660),
    (8, 'AQ', 'ANTARCTICA', 'Antarctica', NULL, NULL),
    (9, 'AG', 'ANTIGUA AND BARBUDA', 'Antigua and Barbuda', 'ATG', 28),
    (10, 'AR', 'ARGENTINA', 'Argentina', 'ARG', 32),
    (11, 'AM', 'ARMENIA', 'Armenia', 'ARM', 51),
    (12, 'AW', 'ARUBA', 'Aruba', 'ABW', 533),
    (13, 'AU', 'AUSTRALIA', 'Australia', 'AUS', 36),
    (14, 'AT', 'AUSTRIA', 'Austria', 'AUT', 40),
    (15, 'AZ', 'AZERBAIJAN', 'Azerbaijan', 'AZE', 31),
    (16, 'BS', 'BAHAMAS', 'Bahamas', 'BHS', 44),
    (17, 'BH', 'BAHRAIN', 'Bahrain', 'BHR', 48),
    (18, 'BD', 'BANGLADESH', 'Bangladesh', 'BGD', 50),
    (19, 'BB', 'BARBADOS', 'Barbados', 'BRB', 52),
    (20, 'BY', 'BELARUS', 'Belarus', 'BLR', 112),
    (21, 'BE', 'BELGIUM', 'Belgium', 'BEL', 56),
    (22, 'BZ', 'BELIZE', 'Belize', 'BLZ', 84),
    (23, 'BJ', 'BENIN', 'Benin', 'BEN', 204),
    (24, 'BM', 'BERMUDA', 'Bermuda', 'BMU', 60),
    (25, 'BT', 'BHUTAN', 'Bhutan', 'BTN', 64),
    (26, 'BO', 'BOLIVIA', 'Bolivia', 'BOL', 68),
    (27, 'BA', 'BOSNIA AND HERZEGOVINA', 'Bosnia and Herzegovina', 'BIH', 70),
    (28, 'BW', 'BOTSWANA', 'Botswana', 'BWA', 72),
    (29, 'BV', 'BOUVET ISLAND', 'Bouvet Island', NULL, NULL),
    (30, 'BR', 'BRAZIL', 'Brazil', 'BRA', 76),
    (31, 'IO', 'BRITISH INDIAN OCEAN TERRITORY', 'British Indian Ocean Territory', NULL, NULL),
    (32, 'BN', 'BRUNEI DARUSSALAM', 'Brunei Darussalam', 'BRN', 96),
    (33, 'BG', 'BULGARIA', 'Bulgaria', 'BGR', 100),
    (34, 'BF', 'BURKINA FASO', 'Burkina Faso', 'BFA', 854),
    (35, 'BI', 'BURUNDI', 'Burundi', 'BDI', 108),
    (36, 'KH', 'CAMBODIA', 'Cambodia', 'KHM', 116),
    (37, 'CM', 'CAMEROON', 'Cameroon', 'CMR', 120),
    (38, 'CA', 'CANADA', 'Canada', 'CAN', 124),
    (39, 'CV', 'CAPE VERDE', 'Cape Verde', 'CPV', 132),
    (40, 'KY', 'CAYMAN ISLANDS', 'Cayman Islands', 'CYM', 136),
    (41, 'CF', 'CENTRAL AFRICAN REPUBLIC', 'Central African Republic', 'CAF', 140),
    (42, 'TD', 'CHAD', 'Chad', 'TCD', 148),
    (43, 'CL', 'CHILE', 'Chile', 'CHL', 152),
    (44, 'CN', 'CHINA', 'China', 'CHN', 156),
    (45, 'CX', 'CHRISTMAS ISLAND', 'Christmas Island', NULL, NULL),
    (46, 'CC', 'COCOS (KEELING) ISLANDS', 'Cocos (Keeling) Islands', NULL, NULL),
    (47, 'CO', 'COLOMBIA', 'Colombia', 'COL', 170),
    (48, 'KM', 'COMOROS', 'Comoros', 'COM', 174),
    (49, 'CG', 'CONGO', 'Congo', 'COG', 178),
    (50, 'CD', 'CONGO, THE DEMOCRATIC REPUBLIC OF THE', 'Congo, the Democratic Republic of the', 'COD', 180),
    (51, 'CK', 'COOK ISLANDS', 'Cook Islands', 'COK', 184),
    (52, 'CR', 'COSTA RICA', 'Costa Rica', 'CRI', 188),
    (53, 'CI', 'COTE D''IVOIRE', 'Cote D''Ivoire', 'CIV', 384),
    (54, 'HR', 'CROATIA', 'Croatia', 'HRV', 191),
    (55, 'CU', 'CUBA', 'Cuba', 'CUB', 192),
    (56, 'CY', 'CYPRUS', 'Cyprus', 'CYP', 196),
    (57, 'CZ', 'CZECH REPUBLIC', 'Czech Republic', 'CZE', 203),
    (58, 'DK', 'DENMARK', 'Denmark', 'DNK', 208),
    (59, 'DJ', 'DJIBOUTI', 'Djibouti', 'DJI', 262),
    (60, 'DM', 'DOMINICA', 'Dominica', 'DMA', 212),
    (61, 'DO', 'DOMINICAN REPUBLIC', 'Dominican Republic', 'DOM', 214),
    (62, 'EC', 'ECUADOR', 'Ecuador', 'ECU', 218),
    (63, 'EG', 'EGYPT', 'Egypt', 'EGY', 818),
    (64, 'SV', 'EL SALVADOR', 'El Salvador', 'SLV', 222),
    (65, 'GQ', 'EQUATORIAL GUINEA', 'Equatorial Guinea', 'GNQ', 226),
    (66, 'ER', 'ERITREA', 'Eritrea', 'ERI', 232),
    (67, 'EE', 'ESTONIA', 'Estonia', 'EST', 233),
    (68, 'ET', 'ETHIOPIA', 'Ethiopia', 'ETH', 231),
    (69, 'FK', 'FALKLAND ISLANDS (MALVINAS)', 'Falkland Islands (Malvinas)', 'FLK', 238),
    (70, 'FO', 'FAROE ISLANDS', 'Faroe Islands', 'FRO', 234),
    (71, 'FJ', 'FIJI', 'Fiji', 'FJI', 242),
    (72, 'FI', 'FINLAND', 'Finland', 'FIN', 246),
    (73, 'FR', 'FRANCE', 'France', 'FRA', 250),
    (74, 'GF', 'FRENCH GUIANA', 'French Guiana', 'GUF', 254),
    (75, 'PF', 'FRENCH POLYNESIA', 'French Polynesia', 'PYF', 258),
    (76, 'TF', 'FRENCH SOUTHERN TERRITORIES', 'French Southern Territories', NULL, NULL),
    (77, 'GA', 'GABON', 'Gabon', 'GAB', 266),
    (78, 'GM', 'GAMBIA', 'Gambia', 'GMB', 270),
    (79, 'GE', 'GEORGIA', 'Georgia', 'GEO', 268),
    (80, 'DE', 'GERMANY', 'Germany', 'DEU', 276),
    (81, 'GH', 'GHANA', 'Ghana', 'GHA', 288),
    (82, 'GI', 'GIBRALTAR', 'Gibraltar', 'GIB', 292),
    (83, 'GR', 'GREECE', 'Greece', 'GRC', 300),
    (84, 'GL', 'GREENLAND', 'Greenland', 'GRL', 304),
    (85, 'GD', 'GRENADA', 'Grenada', 'GRD', 308),
    (86, 'GP', 'GUADELOUPE', 'Guadeloupe', 'GLP', 312),
    (87, 'GU', 'GUAM', 'Guam', 'GUM', 316),
    (88, 'GT', 'GUATEMALA', 'Guatemala', 'GTM', 320),
    (89, 'GN', 'GUINEA', 'Guinea', 'GIN', 324),
    (90, 'GW', 'GUINEA-BISSAU', 'Guinea-Bissau', 'GNB', 624),
    (91, 'GY', 'GUYANA', 'Guyana', 'GUY', 328),
    (92, 'HT', 'HAITI', 'Haiti', 'HTI', 332),
    (93, 'HM', 'HEARD ISLAND AND MCDONALD ISLANDS', 'Heard Island and Mcdonald Islands', NULL, NULL),
    (94, 'VA', 'HOLY SEE (VATICAN CITY STATE)', 'Holy See (Vatican City State)', 'VAT', 336),
    (95, 'HN', 'HONDURAS', 'Honduras', 'HND', 340),
    (96, 'HK', 'HONG KONG', 'Hong Kong', 'HKG', 344),
    (97, 'HU', 'HUNGARY', 'Hungary', 'HUN', 348),
    (98, 'IS', 'ICELAND', 'Iceland', 'ISL', 352),
    (99, 'IN', 'INDIA', 'India', 'IND', 356),
    (100, 'ID', 'INDONESIA', 'Indonesia', 'IDN', 360),
    (101, 'IR', 'IRAN, ISLAMIC REPUBLIC OF', 'Iran, Islamic Republic of', 'IRN', 364),
    (102, 'IQ', 'IRAQ', 'Iraq', 'IRQ', 368),
    (103, 'IE', 'IRELAND', 'Ireland', 'IRL', 372),
    (104, 'IL', 'ISRAEL', 'Israel', 'ISR', 376),
    (105, 'IT', 'ITALY', 'Italy', 'ITA', 380),
    (106, 'JM', 'JAMAICA', 'Jamaica', 'JAM', 388),
    (107, 'JP', 'JAPAN', 'Japan', 'JPN', 392),
    (108, 'JO', 'JORDAN', 'Jordan', 'JOR', 400),
    (109, 'KZ', 'KAZAKHSTAN', 'Kazakhstan', 'KAZ', 398),
    (110, 'KE', 'KENYA', 'Kenya', 'KEN', 404),
    (111, 'KI', 'KIRIBATI', 'Kiribati', 'KIR', 296),
    (112, 'KP', 'KOREA, DEMOCRATIC PEOPLE''S REPUBLIC OF', 'Korea, Democratic People''s Republic of', 'PRK', 408),
    (113, 'KR', 'KOREA, REPUBLIC OF', 'Korea, Republic of', 'KOR', 410),
    (114, 'KW', 'KUWAIT', 'Kuwait', 'KWT', 414),
    (115, 'KG', 'KYRGYZSTAN', 'Kyrgyzstan', 'KGZ', 417),
    (116, 'LA', 'LAO PEOPLE''S DEMOCRATIC REPUBLIC', 'Lao People''s Democratic Republic', 'LAO', 418),
    (117, 'LV', 'LATVIA', 'Latvia', 'LVA', 428),
    (118, 'LB', 'LEBANON', 'Lebanon', 'LBN', 422),
    (119, 'LS', 'LESOTHO', 'Lesotho', 'LSO', 426),
    (120, 'LR', 'LIBERIA', 'Liberia', 'LBR', 430),
    (121, 'LY', 'LIBYAN ARAB JAMAHIRIYA', 'Libyan Arab Jamahiriya', 'LBY', 434),
    (122, 'LI', 'LIECHTENSTEIN', 'Liechtenstein', 'LIE', 438),
    (123, 'LT', 'LITHUANIA', 'Lithuania', 'LTU', 440),
    (124, 'LU', 'LUXEMBOURG', 'Luxembourg', 'LUX', 442),
    (125, 'MO', 'MACAO', 'Macao', 'MAC', 446),
    (126, 'MK', 'MACEDONIA, THE FORMER YUGOSLAV REPUBLIC OF', 'Macedonia, the Former Yugoslav Republic of', 'MKD', 807),
    (127, 'MG', 'MADAGASCAR', 'Madagascar', 'MDG', 450),
    (128, 'MW', 'MALAWI', 'Malawi', 'MWI', 454),
    (129, 'MY', 'MALAYSIA', 'Malaysia', 'MYS', 458),
    (130, 'MV', 'MALDIVES', 'Maldives', 'MDV', 462),
    (131, 'ML', 'MALI', 'Mali', 'MLI', 466),
    (132, 'MT', 'MALTA', 'Malta', 'MLT', 470),
    (133, 'MH', 'MARSHALL ISLANDS', 'Marshall Islands', 'MHL', 584),
    (134, 'MQ', 'MARTINIQUE', 'Martinique', 'MTQ', 474),
    (135, 'MR', 'MAURITANIA', 'Mauritania', 'MRT', 478),
    (136, 'MU', 'MAURITIUS', 'Mauritius', 'MUS', 480),
    (137, 'YT', 'MAYOTTE', 'Mayotte', NULL, NULL),
    (138, 'MX', 'MEXICO', 'Mexico', 'MEX', 484),
    (139, 'FM', 'MICRONESIA, FEDERATED STATES OF', 'Micronesia, Federated States of', 'FSM', 583),
    (140, 'MD', 'MOLDOVA, REPUBLIC OF', 'Moldova, Republic of', 'MDA', 498),
    (141, 'MC', 'MONACO', 'Monaco', 'MCO', 492),
    (142, 'MN', 'MONGOLIA', 'Mongolia', 'MNG', 496),
    (143, 'MS', 'MONTSERRAT', 'Montserrat', 'MSR', 500),
    (144, 'MA', 'MOROCCO', 'Morocco', 'MAR', 504),
    (145, 'MZ', 'MOZAMBIQUE', 'Mozambique', 'MOZ', 508),
    (146, 'MM', 'MYANMAR', 'Myanmar', 'MMR', 104),
    (147, 'NA', 'NAMIBIA', 'Namibia', 'NAM', 516),
    (148, 'NR', 'NAURU', 'Nauru', 'NRU', 520),
    (149, 'NP', 'NEPAL', 'Nepal', 'NPL', 524),
    (150, 'NL', 'NETHERLANDS', 'Netherlands', 'NLD', 528),
    (151, 'AN', 'NETHERLANDS ANTILLES', 'Netherlands Antilles', 'ANT', 530),
    (152, 'NC', 'NEW CALEDONIA', 'New Caledonia', 'NCL', 540),
    (153, 'NZ', 'NEW ZEALAND', 'New Zealand', 'NZL', 554),
    (154, 'NI', 'NICARAGUA', 'Nicaragua', 'NIC', 558),
    (155, 'NE', 'NIGER', 'Niger', 'NER', 562),
    (156, 'NG', 'NIGERIA', 'Nigeria', 'NGA', 566),
    (157, 'NU', 'NIUE', 'Niue', 'NIU', 570),
    (158, 'NF', 'NORFOLK ISLAND', 'Norfolk Island', 'NFK', 574),
    (159, 'MP', 'NORTHERN MARIANA ISLANDS', 'Northern Mariana Islands', 'MNP', 580),
    (160, 'NO', 'NORWAY', 'Norway', 'NOR', 578),
    (161, 'OM', 'OMAN', 'Oman', 'OMN', 512),
    (162, 'PK', 'PAKISTAN', 'Pakistan', 'PAK', 586),
    (163, 'PW', 'PALAU', 'Palau', 'PLW', 585),
    (164, 'PS', 'PALESTINIAN TERRITORY, OCCUPIED', 'Palestinian Territory, Occupied', NULL, NULL),
    (165, 'PA', 'PANAMA', 'Panama', 'PAN', 591),
    (166, 'PG', 'PAPUA NEW GUINEA', 'Papua New Guinea', 'PNG', 598),
    (167, 'PY', 'PARAGUAY', 'Paraguay', 'PRY', 600),
    (168, 'PE', 'PERU', 'Peru', 'PER', 604),
    (169, 'PH', 'PHILIPPINES', 'Philippines', 'PHL', 608),
    (170, 'PN', 'PITCAIRN', 'Pitcairn', 'PCN', 612),
    (171, 'PL', 'POLAND', 'Poland', 'POL', 616),
    (172, 'PT', 'PORTUGAL', 'Portugal', 'PRT', 620),
    (173, 'PR', 'PUERTO RICO', 'Puerto Rico', 'PRI', 630),
    (174, 'QA', 'QATAR', 'Qatar', 'QAT', 634),
    (175, 'RE', 'REUNION', 'Reunion', 'REU', 638),
    (176, 'RO', 'ROMANIA', 'Romania', 'ROM', 642),
    (177, 'RU', 'RUSSIAN FEDERATION', 'Russian Federation', 'RUS', 643),
    (178, 'RW', 'RWANDA', 'Rwanda', 'RWA', 646),
    (179, 'SH', 'SAINT HELENA', 'Saint Helena', 'SHN', 654),
    (180, 'KN', 'SAINT KITTS AND NEVIS', 'Saint Kitts and Nevis', 'KNA', 659),
    (181, 'LC', 'SAINT LUCIA', 'Saint Lucia', 'LCA', 662),
    (182, 'PM', 'SAINT PIERRE AND MIQUELON', 'Saint Pierre and Miquelon', 'SPM', 666),
    (183, 'VC', 'SAINT VINCENT AND THE GRENADINES', 'Saint Vincent and the Grenadines', 'VCT', 670),
    (184, 'WS', 'SAMOA', 'Samoa', 'WSM', 882),
    (185, 'SM', 'SAN MARINO', 'San Marino', 'SMR', 674),
    (186, 'ST', 'SAO TOME AND PRINCIPE', 'Sao Tome and Principe', 'STP', 678),
    (187, 'SA', 'SAUDI ARABIA', 'Saudi Arabia', 'SAU', 682),
    (188, 'SN', 'SENEGAL', 'Senegal', 'SEN', 686),
    (189, 'CS', 'SERBIA AND MONTENEGRO', 'Serbia and Montenegro', NULL, NULL),
    (190, 'SC', 'SEYCHELLES', 'Seychelles', 'SYC', 690),
    (191, 'SL', 'SIERRA LEONE', 'Sierra Leone', 'SLE', 694),
    (192, 'SG', 'SINGAPORE', 'Singapore', 'SGP', 702),
    (193, 'SK', 'SLOVAKIA', 'Slovakia', 'SVK', 703),
    (194, 'SI', 'SLOVENIA', 'Slovenia', 'SVN', 705),
    (195, 'SB', 'SOLOMON ISLANDS', 'Solomon Islands', 'SLB', 90),
    (196, 'SO', 'SOMALIA', 'Somalia', 'SOM', 706),
    (197, 'ZA', 'SOUTH AFRICA', 'South Africa', 'ZAF', 710),
    (198, 'GS', 'SOUTH GEORGIA AND THE SOUTH SANDWICH ISLANDS', 'South Georgia and the South Sandwich Islands', NULL, NULL),
    (199, 'ES', 'SPAIN', 'Spain', 'ESP', 724),
    (200, 'LK', 'SRI LANKA', 'Sri Lanka', 'LKA', 144),
    (201, 'SD', 'SUDAN', 'Sudan', 'SDN', 736),
    (202, 'SR', 'SURINAME', 'Suriname', 'SUR', 740),
    (203, 'SJ', 'SVALBARD AND JAN MAYEN', 'Svalbard and Jan Mayen', 'SJM', 744),
    (204, 'SZ', 'SWAZILAND', 'Swaziland', 'SWZ', 748),
    (205, 'SE', 'SWEDEN', 'Sweden', 'SWE', 752),
    (206, 'CH', 'SWITZERLAND', 'Switzerland', 'CHE', 756),
    (207, 'SY', 'SYRIAN ARAB REPUBLIC', 'Syrian Arab Republic', 'SYR', 760),
    (208, 'TW', 'TAIWAN, PROVINCE OF CHINA', 'Taiwan, Province of China', 'TWN', 158),
    (209, 'TJ', 'TAJIKISTAN', 'Tajikistan', 'TJK', 762),
    (210, 'TZ', 'TANZANIA, UNITED REPUBLIC OF', 'Tanzania, United Republic of', 'TZA', 834),
    (211, 'TH', 'THAILAND', 'Thailand', 'THA', 764),
    (212, 'TL', 'TIMOR-LESTE', 'Timor-Leste', NULL, NULL),
    (213, 'TG', 'TOGO', 'Togo', 'TGO', 768),
    (214, 'TK', 'TOKELAU', 'Tokelau', 'TKL', 772),
    (215, 'TO', 'TONGA', 'Tonga', 'TON', 776),
    (216, 'TT', 'TRINIDAD AND TOBAGO', 'Trinidad and Tobago', 'TTO', 780),
    (217, 'TN', 'TUNISIA', 'Tunisia', 'TUN', 788),
    (218, 'TR', 'TURKEY', 'Turkey', 'TUR', 792),
    (219, 'TM', 'TURKMENISTAN', 'Turkmenistan', 'TKM', 795),
    (220, 'TC', 'TURKS AND CAICOS ISLANDS', 'Turks and Caicos Islands', 'TCA', 796),
    (221, 'TV', 'TUVALU', 'Tuvalu', 'TUV', 798),
    (222, 'UG', 'UGANDA', 'Uganda', 'UGA', 800),
    (223, 'UA', 'UKRAINE', 'Ukraine', 'UKR', 804),
    (224, 'AE', 'UNITED ARAB EMIRATES', 'United Arab Emirates', 'ARE', 784),
    (225, 'GB', 'UNITED KINGDOM', 'United Kingdom', 'GBR', 826),
    (226, 'US', 'UNITED STATES', 'United States', 'USA', 840),
    (227, 'UM', 'UNITED STATES MINOR OUTLYING ISLANDS', 'United States Minor Outlying Islands', NULL, NULL),
    (228, 'UY', 'URUGUAY', 'Uruguay', 'URY', 858),
    (229, 'UZ', 'UZBEKISTAN', 'Uzbekistan', 'UZB', 860),
    (230, 'VU', 'VANUATU', 'Vanuatu', 'VUT', 548),
    (231, 'VE', 'VENEZUELA', 'Venezuela', 'VEN', 862),
    (232, 'VN', 'VIET NAM', 'Viet Nam', 'VNM', 704),
    (233, 'VG', 'VIRGIN ISLANDS, BRITISH', 'Virgin Islands, British', 'VGB', 92),
    (234, 'VI', 'VIRGIN ISLANDS, U.S.', 'Virgin Islands, U.s.', 'VIR', 850),
    (235, 'WF', 'WALLIS AND FUTUNA', 'Wallis and Futuna', 'WLF', 876),
    (236, 'EH', 'WESTERN SAHARA', 'Western Sahara', 'ESH', 732),
    (237, 'YE', 'YEMEN', 'Yemen', 'YEM', 887),
    (238, 'ZM', 'ZAMBIA', 'Zambia', 'ZMB', 894),
    (239, 'ZW', 'ZIMBABWE', 'Zimbabwe', 'ZWE', 716)
ON CONFLICT (iso) DO NOTHING;
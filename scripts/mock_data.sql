INSERT INTO app.users (email, password) VALUES
    ('john.doe@example.com', 'password'),
    ('jane.smith@example.com', 'password'),
    ('alice.johnson@example.com', 'password')
ON CONFLICT (email) DO NOTHING;

INSERT INTO app.users_info (user_id, first_name, last_name, country_id, phone_code, phone_number) VALUES
    (
        (SELECT id FROM app.users WHERE email = 'john.doe@example.com'),
        'John',
        'Doe',
        72, -- Finland in app.countries
        '+358',
        '442356767'
    ),
    (
        (SELECT id FROM app.users WHERE email = 'jane.smith@example.com'),
        'Jane',
        'Smith',
        225, -- United Kingdom in app.countries
        '+44',
        '2071234567'
    ),
    (
        (SELECT id FROM app.users WHERE email = 'alice.johnson@example.com'),
        'Alice',
        'Johnson',
        73, -- France in app.countries
        '+33',
        '109758351'
    )
ON CONFLICT (user_id) DO NOTHING;
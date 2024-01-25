SELECT u.id, u.email, ui.first_name, ui.last_name, c.nicename
FROM app.users u
JOIN app.users_info ui ON u.id = ui.user_id
JOIN app.countries c ON ui.country_id = c.id;
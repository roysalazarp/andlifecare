SELECT 
    u.id, 
    u.email, 
    c.nicename,
    CONCAT(ui.first_name, ' ', ui.last_name) AS full_name
FROM app.users u
JOIN app.users_info ui ON u.id = ui.user_id
JOIN app.countries c ON ui.country_id = c.id;
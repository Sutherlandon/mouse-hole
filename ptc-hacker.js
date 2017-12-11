for (var i = 0; i < 10; i++) {
    d = new Date(Date.parse("July 15, 2016"));
    day = ("0" + d.getDate()).slice(-2)
    mon = ("0" + (d.getMonth() + 1)).slice(-2)
    next_date = mon + "-" + day + "-" + d.getFullYear()

    Document.getElementById("id_password").value = "N3tworks!@";
    Document.getElementById("id_confirm_password").value = "N3tworks!@";
    Document.getElementById("id_dob").value = next_date;
    Document.getElementById("id_username").value = "Sutherlandon";
    Document.getElementById("fogot_password").submit();
}
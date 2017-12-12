// Spams the Pokemon Trainer CLub Password Reset form with DOB's starting with
// a date and going back one by one
//
// I don't think this actually works, by DOB turns out ot be set as 06-27-1990
// but running this once seemed to work, so it may accepts DOB or Date of 
// signup.
//
// This was run using the Custom Javascript for Websites 2 Chrome extension

// waits 3 seconds then runs
window.setInterval(function(){
	// pick a start date
	d = new Date(Date.parse("July 15, 2016"));                                         

	// Currently only checks the start date because of page reload
	day = ("0" + d.getDate()).slice(-2);                                           
	mon = ("0" + (d.getMonth() + 1)).slice(-2);                                    
	next_date = mon + "-" + day + "-" + d.getFullYear();                           
	console.log(next_date.toString());                                             

	// fill the form and submit
	document.getElementById("id_password").value = "N3tworks!@";                   
	document.getElementById("id_confirm_password").value = "N3tworks!@";           
	document.getElementById("id_dob").value = next_date;                           
	document.getElementById("id_username").value = "Sutherlandon";                 
	document.getElementById("forgot-password").submit();

}, 3000);

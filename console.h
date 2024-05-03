/*
 * initial html file allowing uploads of others into SPIFFS via the webserver
 */
#define cfilename "/console.html"
char consolestr[2000]= \
"<!DOCTYPE html>" \
"<html>" \
"<style>" \
"	.button {" \
"		width: 87px;" \
"	}" \
"</style>" \
"<body>	" \
"	<h2 style=\"text-align:center;\">File Manager</h2>" \
"	<form action='/edit'  target='iframe_b' method='POST' enctype='multipart/form-data' style='width: 400px'>" \
"		<input type='file' name='update'>" \
"		<input class='button'; type='submit' value='Upload'>" \
"	</form>" \
"	<form action=\"/ls\"    target=\"iframe_b\" method=\"POST\">" \
"		<input class='button' type=\"submit\" value=\"List Files\">" \
"	</form>" \
"	<h2 style=\"text-align:center;\">Debug</h2>" \
"	<form action=\"/debug\" target=\"iframe_b\" method=\"POST\">" \
"		<input type=\"text\" name=\"command\" value=\"p\">" \
"		<input type=\"submit\" value=\"Submit\">" \
"	</form>" \
"	<br>" \
"	<iframe height=\"100px\" width=\"400px\" name=\"iframe_b\"></iframe>" \
"</body>" \
"</html>";

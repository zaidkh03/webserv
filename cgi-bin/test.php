#!/usr/bin/php-cgi
<?php
header("Content-Type: text/html");

echo "<html><body>";
echo "<h1>PHP CGI Works!</h1>";
echo "<p>Server: " . $_SERVER['SERVER_NAME'] . "</p>";
echo "<p>Method: " . $_SERVER['REQUEST_METHOD'] . "</p>";
echo "<p>Script: " . $_SERVER['SCRIPT_NAME'] . "</p>";

if (isset($_SERVER['QUERY_STRING'])) {
    echo "<p>Query: " . $_SERVER['QUERY_STRING'] . "</p>";
}

echo "</body></html>";
?>

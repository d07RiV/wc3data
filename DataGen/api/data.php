<?php
if (!preg_match('/^\/api\/(\d+)\.json$/', strtok($_SERVER['REQUEST_URI'], '?'), $matches)) {
  http_response_code(400);
  header('Content-Type: application/json');
  die(json_encode(array('error' => 'Invalid request.')));
}

$id = $matches[1];

$filename = "../work/$id.json.gz";
if (!file_exists($filename)) {
  http_response_code(404);
  header('Content-Type: application/json');
  die(json_encode(array('error' => 'No data for specified version.')));
}

$tm = filemtime($filename);
$out = file_get_contents($filename);
$tmstring = gmdate('D, d M Y H:i:s ', $tm) . 'GMT';
if (isset($_SERVER['HTTP_IF_MODIFIED_SINCE']) && $_SERVER['HTTP_IF_MODIFIED_SINCE'] == $tmstring) {
  header('HTTP/1.1 304 Not Modified');
  exit();
}
//$out = gzencode($out);
header('Content-Type: application/json; charset=utf-8');
header('Content-Encoding: gzip');
header('Last-Modified: ' . $tmstring);
header('Content-Length: ' . strlen($out));
echo $out;
?>
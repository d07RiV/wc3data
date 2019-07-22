<?php
require("common.inc.php");

if (!preg_match('/^\/api\/(images|files)\/([a-fA-F0-9]{16})$/', strtok($_SERVER['REQUEST_URI'], '?'), $matches)) {
  http_response_code(400);
  header('Content-Type: application/json');
  die(json_encode(array('error' => 'Invalid request.')));
}

$tileset = 0;
if (isset($_REQUEST['tileset']) && preg_match('/^[A-Z]$/', $_REQUEST['tileset'])) {
  $tileset = ord($_REQUEST['tileset']) - 64;
}

function load_file($type, $idhi, $idlo) {
  if ($type === 'files') {
    $filename = '../work/files.gzx';
  } else if ($type === 'images') {
    $fileid = $idlo & 7;
    $filename = "../work/images$fileid.gzx";
  }
  if (!file_exists($filename)) {
    return NULL;
  }

  $data = load_gzx($filename, $idhi, $idlo);
  if ($data) {
    $data = array($data[0], filemtime($filename), $data[1]);
  }
  return $data;
}

$type = $matches[1];
$idhi = hexdec(substr($matches[2], 0, 8));
$idlo = hexdec(substr($matches[2], 8, 8));

$res = NULL;

if ($tileset) {
  $idhi2 = $idhi;
  $idlo2 = $idlo;
  $maxv = (2147483648 - $tileset) + 2147483648;
  if ($idlo2 >= $maxv) {
    $idlo2 -= $maxv;
    if ($idhi2 == 4294967295) {
      $idhi2 = 0;
    } else {
      $idhi2 += 1;
    }
  } else {
    $idlo2 += $tileset;
  }
  $res = load_file($type, $idhi2, $idlo2);
}

if (!$res) {
  $res = load_file($type, $idhi, $idlo);
}

if (!$res) {
  header('HTTP/1.1 404 Not Found');
  die('File not found');
}

$tmstring = check_expiry($res[1]);

if ($type === 'images') {
  header('Content-Type: image/png');
} else {
  header('Content-Type: application/octet-stream');
}
header('Last-Modified: ' . $res[1]);
if ($res[2]) {
  header('Content-Encoding: gzip');
}
header('Content-Length: ' . strlen($res[0]));
echo $res[0];
?>
<?php
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

  $handle = fopen($filename, 'rb');
  fseek($handle, 4);
  $count = unpack('V', fread($handle, 4))[1];
  $dir = fread($handle, $count * 20);

  $left = 0;
  $right = $count;
  while ($right > $left) {
    $mid = (int)(($left + $right) / 2);
    $id = unpack('V2', substr($dir, $mid * 20, 8));
    if ($id[1] < 0) $id[1] += 4294967296;
    if ($id[2] < 0) $id[2] += 4294967296;
    if ($id[1] == $idlo && $id[2] == $idhi) {
      break;
    } else if ($id[2] < $idhi || ($id[2] == $idhi && $id[1] < $idlo)) {
      $left = $mid + 1;
    } else {
      $right = $mid;
    }
  }
  if (!isset($id) || $id[1] != $idlo || $id[2] != $idhi) {
    return NULL;
  }
  $filepos = unpack('V3', substr($dir, $mid * 20 + 8, 12));
  fseek($handle, $filepos[1]);
  $out = fread($handle, $filepos[2]);
  fclose($handle);
  return array($out, filemtime($filename), $filepos[3] > $filepos[2]);
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

$tmstring = gmdate('D, d M Y H:i:s ', $res[1]) . 'GMT';
if (isset($_SERVER['HTTP_IF_MODIFIED_SINCE']) && $_SERVER['HTTP_IF_MODIFIED_SINCE'] == $tmstring) {
  header('HTTP/1.1 304 Not Modified');
  exit();
}

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
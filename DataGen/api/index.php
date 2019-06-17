<?php
error_reporting( E_ALL );
ini_set('display_errors', 1);

require('common.inc.php');

$path = '../index.html';
$dataroot = './';

$uri = strtok($_SERVER['REQUEST_URI'], '?');
if (!preg_match('/^\/(\w+)(?:\/.*)?$/', $uri, $matches)) {
  header('HTTP/1.1 404 Not Found');
  die('File not found');
}
$version = $matches[1];
$version_list = json_decode(file_get_contents($dataroot . 'versions.json'), TRUE);

if (isset($version_list['custom'][$version])) {
  $map_path = $version_list['custom'][$version]['data'];
  $map_name = $version_list['custom'][$version]['name'];
  $map_name = preg_replace('/\|(c[0-9a-fA-F]{6,8}|r)/', '', $map_name);
} else {
  $map_path = NULL;
  $map_name = NULL;
}

$title = 'WC3 Data';
if ($map_name) {
  $title = "$title - $map_name";
}

function find_name($listfile, $value, $imExt) {
  foreach (explode("\n", $listfile) as $name) {
    $name = trim($name);
    $hash = pathHash($name, $imExt);
    if ($hash === $value) {
      $slash = strrpos($name, "\\");
      if ($slash != FALSE) {
        $name = substr($name, $slash + 1);
      }
      return $name;
    }
  }
  return NULL;
}

if (preg_match('/^\/\w+\/(files|unit|item|destructible|doodad|ability|buff|upgrade)\/(\w+)$/', $uri, $matches)) {
  if ($matches[1] === 'files') {
    if (!$map_path) {
      $name = find_name(file_get_contents($dataroot . 'rootlist.txt'), $matches[2], TRUE);
    } else {
      $name = find_name(unpack_gzx($dataroot . $map_path, 'listfile.txt'), $matches[2], FALSE);
    }
    if ($name) {
      $title = "$title - $name";
    }
  } else {
    if ($map_path) {
      $odata = unpack_gzx($dataroot . $map_path, 'objects.json');
    } else if (!file_exists($dataroot . "$version.json.gz")) {
      header('HTTP/1.1 404 Not Found');
      die('File not found');
    } else {
      $odata = file_get_contents($dataroot . "$version.json.gz");
      $odata = gzdecode($odata);
    }
    if ($odata) {
      $odata = json_decode($odata, TRUE);
      foreach($odata['objects'] as $obj) {
        if ($obj['id'] == $matches[2]) {
          $title = "$title - " . $obj['name'];
          break;
        }
      }
    }
  }
}

$tmstring = check_expiry(filemtime($path));

$content = file_get_contents($path);
$content = preg_replace('/<title>.*<\/title>/', "<title>$title</title>", $content);
$content = gzencode($content);
header('Content-Type: text/html; charset=utf-8');
header('Content-Encoding: gzip');
header('Last-Modified: ' . $tmstring);
header('Content-Length: ' . strlen($content));
echo $content;
?>
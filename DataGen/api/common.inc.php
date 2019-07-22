<?php
function load_gzx($filename, $idhi, $idlo) {
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
    fclose($handle);
    return NULL;
  }
  $filepos = unpack('V3', substr($dir, $mid * 20 + 8, 12));
  fseek($handle, $filepos[1]);
  $out = fread($handle, $filepos[2]);
  fclose($handle);
  return array($out, $filepos[3] > $filepos[2]);
}
function check_expiry($time) {
  $tmstring = gmdate('D, d M Y H:i:s ', $time) . 'GMT';
  if (isset($_SERVER['HTTP_IF_MODIFIED_SINCE']) && $_SERVER['HTTP_IF_MODIFIED_SINCE'] == $tmstring) {
    header('HTTP/1.1 304 Not Modified');
    exit();
  }
  return $tmstring;
}
function load_gzx_str($filename, $str) {
  $idhi = hexdec(substr($str, 0, 8));
  $idlo = hexdec(substr($str, 8, 8));
  return load_gzx($filename, $idhi, $idlo);
}

function pathHashTyped($name, $hashType) {
  global $cryptTable;
  if (!isset($cryptTable)) {
    $cryptTable = array();
    $seed = 0x00100001;
    for ($i = 0; $i < 256; $i++) {
      for ($j = $i; $j < 1280; $j += 256) {
        $seed = ($seed * 125 + 3) % 0x2AAAAB;
        $a = ($seed & 0xFFFF) << 16;
        $seed = ($seed * 125 + 3) % 0x2AAAAB;
        $b = ($seed & 0xFFFF);
        $cryptTable[$j] = $a | $b;
      }
    }
  }
  $seed1 = 0x7FED7FED;
  $seed2 = 0xEEEEEEEE;
  $bytes = unpack('C*', $name);
  foreach($bytes as $ch) {
    if ($ch >= 0x61 && $ch <= 0x7A) {
      $ch -= 0x20;
    }
    if ($ch === 0x2F) {
      $ch = 0x5C;
    }
    $seed1 = $cryptTable[$hashType * 256 + $ch] ^ (($seed1 + $seed2) & 0xFFFFFFFF);
    $seed2 = ($ch + $seed1 + $seed2 * 33 + 3) & 0xFFFFFFFF;
  }
  return $seed1;
}

function dechex8($val) {
  $str = dechex($val);
  return str_repeat('0', 8 - strlen($str)) . $str;
}

function pathHash($name, $imExt) {
  if ($imExt && preg_match('/^(.*)\.(?:blp|dds|gif|jpg|jpeg|png|tga)$/', $name, $m)) {
    $name = $m[1];
  }
  $delta = 0;
  if (preg_match('/^([A-Z])\.w3mod:(.*)$/', $name, $m)) {
    $delta = ord($m[1]) - 64;
    $name = $m[2];
  }
  $u0 = pathHashTyped($name, 1);
  $u1 = pathHashTyped($name, 2);
  if ($delta) {
    $u0 += $delta;
    $u1 += ($delta >> 32);
    $u0 &= 0xFFFFFFFF;
    $u1 &= 0xFFFFFFFF;
  }
  return dechex8($u1) . dechex8($u0);
}

function unpack_gzx($path, $name) {
  $hash = pathHash($name, FALSE);
  $loaded = load_gzx_str($path, $hash);
  if (!$loaded) {
    return NULL;
  }
  if ($loaded[1]) {
    return gzdecode($loaded[0]);
  }
  return $loaded[0];
}
?>
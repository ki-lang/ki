<?php

$dir = __DIR__;

$versions = ['1', '1.1', '1.2', '1.3', '1.4', '1.5', '1.6', '1.7', '1.8', '1.9', '1.10', '1.11', '1.12', '1.13', '1.14', '1.15', '1.16', '1.17', '1.18', '1.19', '1.20'];


foreach ($versions as $v) {
	$url = "https://raw.githubusercontent.com/golang/go/master/api/go$v.txt";
	$file = $dir . '/go-api-' . $v . '.txt';
	if (!file_exists($file)) {
		$data = file_get_contents($url);
		file_put_contents($file, $data);
	}
}

$defines = [];

foreach ($versions as $v) {
	$file = $dir . '/go-api-' . $v . '.txt';

	$handle = fopen($file, "r");
	if ($handle) {
		while (($line = fgets($handle)) !== false) {
			$ex = explode(' ', $line);
			if (count($ex) == 7 && $ex[5] == '=') {
				// Const
				$os = trim($ex[2], "(),");
				$key = strtoupper($ex[4]);
				$value = trim($ex[6]);

				if (!intval($value)) continue;

				if (!isset($defines[$os])) {
					$defines[$os] = [
						'const' => [],
						'var' => [],
					];
				}
				$defines[$os]['const'][$key] = $value;
			}
			if (count($ex) == 6 && $ex[3] == 'var') {
				// Global var

				$os = trim($ex[2], "(),");
				$key = strtolower($ex[4]);
				$type = trim($ex[5]);

				if (!isset($defines[$os])) {
					$defines[$os] = [
						'const' => [],
						'var' => [],
					];
				}

				$defines[$os]['var'][$key] = $type;
			}
		}
		fclose($handle);
	}
}


$out_dir = $dir . '/out';
if (!file_exists($out_dir))
	mkdir($out_dir);

foreach ($defines as $os => $defs) {
	$lines = [];
	$globals = [];
	foreach ($defs['const'] as $k => $v) {
		$lines[] = "    $k: $v";
	}
	foreach ($defs['var'] as $k => $v) {
		$globals[] = "global $k : $v;";
	}
	$out = "\n" . implode("\n", $globals) . "\n\nenum OS {\n" . implode("\n", $lines) . "\n}\n";
	file_put_contents($out_dir . '/' . $os . '.kh', $out);
}

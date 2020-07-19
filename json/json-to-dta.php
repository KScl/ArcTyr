<?php

// This script is a little messy, but it enables some flexibility with defining data for ArcTyr

$dirsep = DIRECTORY_SEPARATOR;
$assembly_file = './assembly.json';
$output_dir = '../arcdata/';

function write_byte($f, $q) {
	fwrite($f, pack('C', $q));
}

function write_word($f, $q) {
	fwrite($f, pack('v', $q));
}

// Ensure the first element is always "none"
// because Tyrian assumes things start at 1, generally
$all_shots = array('__None__' => []);
$all_ships = array('__None__' => ['Name' => 'None']);
$all_ports = array('__None__' => ['Name' => 'None']);
$all_options = array('__None__' => ['Name' => 'None']);
$all_specials = array('__None__' => ['Name' => 'None']);
$information = array();
$files_to_load = array();

function read_json($file) {
	global $files_to_load, $output_dir;
	global $all_shots, $all_ships, $all_ports, $all_options, $all_specials, $information;
	echo("Now reading {$file} ...\n");

	$jsondata = file_get_contents($file, "r");
	$jsondata = preg_replace("#//.*\n#", '', $jsondata); // remove comments because fuck JSON
	if (($object = json_decode($jsondata, true)) == NULL)
		die("{$file}: JSON error: ".json_last_error_msg()."\n");

	foreach ($object as $type => $data)
	{
		switch ($type) {
			case 'Shots':
				$all_shots = array_merge($all_shots, $data);
				printf(" * %d shot%s loaded.\n", count($data), count($data) == 1 ? "" : "s");
				break;
			case 'Ships':
				$all_ships = array_merge($all_ships, $data);
				printf(" * %d ship%s loaded.\n", count($data), count($data) == 1 ? "" : "s");
				break;
			case 'Ports':
				$all_ports = array_merge($all_ports, $data);
				printf(" * %d port%s loaded.\n", count($data), count($data) == 1 ? "" : "s");
				break;
			case 'Options':
				$all_options = array_merge($all_options, $data);
				printf(" * %d option%s loaded.\n", count($data), count($data) == 1 ? "" : "s");
				break;
			case 'Specials':
				$all_specials = array_merge($all_specials, $data);
				printf(" * %d special%s loaded.\n", count($data), count($data) == 1 ? "" : "s");
				break;
			// Meta types
			case 'Info':
				$information = array_merge($information, $data);
				break;
			case 'Import':
				$diffdata = array_diff($data, $files_to_load);
				$files_to_load = array_merge($files_to_load, $diffdata);
				printf(" * %d additional file%s to load.\n", count($diffdata), count($diffdata) == 1 ? "" : "s");
				break;
			case 'Output':
				$output_dir = $data;
				echo(" * Output directory is now {$output_dir}.\n");
				break;
			default: die("{$file}: Invalid type {$type}\n");
		}
	}
}

function open_output($file) {
	global $output_dir;
	return fopen("{$output_dir}{$file}", "wb+");
}

function get_default($obj, $key, $default) {
	if (!isset($obj[$key]))
		return $default;
	return $obj[$key];
}

// Read JSON files in
// ----------------------------------------------------------------------------

read_json($assembly_file);

// Don't use foreach because array contents WILL change
for ($filenum = 0; $filenum < count($files_to_load); ++$filenum)
	read_json($files_to_load[$filenum]);

// Referenced in many other places.
$shot_ids = array_keys($all_shots);
$port_ids = array_keys($all_ports);
$special_ids = array_keys($all_specials);
$ship_ids = array_keys($all_ships);
$option_ids = array_keys($all_options);

// Output data files from combined JSON objects
// ----------------------------------------------------------------------------

$output = open_output('arcshot.dta');
$entries = count($all_shots) - 1;
write_word($output, $entries);

foreach ($all_shots as $id => $shot) {
	// to Tyrian format
	$at = array();
	$dl = array();
	$sx = array();
	$sy = array();
	$bx = array();
	$by = array();
	$sg = array();
	$ay = array();
	$ax = array();
	$cr = array();
	// end Tyrian format

	$max = 0;
	if (isset($shot['Pieces'])) {
		foreach ($shot['Pieces'] as $piece) {
			$at[$max] = get_default($piece, 'AttackPower', 0);
			$dl[$max] = get_default($piece, 'Duration', 255);
			$sx[$max] = get_default($piece, 'MoveX', 0);
			$sy[$max] = get_default($piece, 'MoveY', 0);
			$bx[$max] = get_default($piece, 'X', 0);
			$by[$max] = get_default($piece, 'Y', 0);
			$sg[$max] = get_default($piece, 'Sprite', 0);
			$ay[$max] = get_default($piece, 'AccelY', 0);
			$ax[$max] = get_default($piece, 'AccelX', 0);
			$cr[$max] = get_default($piece, 'CircleParams', 0);

			// Attack Power hacks
			if ($at[$max] === 'Ice') $at[$max] = 99;
			elseif (is_array($at[$max])) {
				if (isset($at[$max]['Infinite'])) {
					$temp = $at[$max]['Infinite'];
					if ($temp < 0 || $temp > 5) die("Invalid shot piece {$id}: Infinite:{$temp} is out of range\n");
					$at[$max] = 250 + $temp;
				}
				elseif (isset($at[$max]['Shrapnel'])) {
					$temp = array_search($at[$max]['Shrapnel'], $shot_ids);
					if ($temp === FALSE) die("Invalid shot piece {$id}: Shrapnel:{$at[$max]['Shrapnel']} doesn't match another shot\n");
					if ($temp < 0 || $temp > 99) die("Invalid shot piece {$id}: Shrapnel:{$at[$max]['Shrapnel']} is out of range\n");
					$at[$max] = 100 + $temp;
				}
			}
			if (!is_int($at[$max]))
				die("Invalid shot piece {$id}: Unknown value {$at[$max]}\n");

			// Duration hacks
			if ($dl[$max] === 'NoTrail') $dl[$max] = 121;
			elseif (is_array($dl[$max])) {
				if (isset($dl[$max]['Carry'])) {
					if ($dl[$max]['Carry'] == 'X') $dl[$max] = 98;
					elseif ($dl[$max]['Carry'] == 'XY') $dl[$max] = 99;
					elseif ($dl[$max]['Carry'] == 'Y') $dl[$max] = 100;
					else die("Invalid shot piece {$id}: Carry:{$dl[$max]['Carry']} is invalid\n");
				}
				elseif (isset($dl[$max]['SetAni'])) {
					$temp = $dl[$max]['SetAni'];
					if ($temp < 1 || $temp > 20) die("Invalid shot piece {$id}: SetAni:{$dl[$max]['SetAni']} is out of range\n");
					$dl[$max] = 100 + $temp;
				}
			}
			if (!is_int($dl[$max]))
				die("Invalid shot piece {$id}: Unknown value {$dl[$max]}\n");

			// Sprite hacks
			if (is_array($sg[$max])) {
				if (isset($sg[$max]['Special'])) {
					$temp = $sg[$max]['Special'] + 60000;
					$sg[$max] = $temp;
				}
				elseif (isset($sg[$max]['ArcTyr'])) {
					$temp = $sg[$max]['ArcTyr'] + 1000;
					$sg[$max] = $temp;
				}
				elseif (isset($sg[$max]['Tyrian2000'])) {
					$temp = $sg[$max]['Tyrian2000'];
					$sg[$max] = $temp;
				}
			}
			if (!is_int($sg[$max]))
				die("Invalid shot piece {$id}: Unknown value {$sg[$max]}\n");

			// MoveX hacks
			if ($sx[$max] === 'FollowShip') $sx[$max] = 101;
			else if (is_array($sx[$max]) && isset($sx[$max]['FollowX'])) {
				$temp = 120 + $sx[$max]['FollowX'];
				if ($temp < 101 || $temp > 255) die("Invalid shot piece {$id}: FollowX:{$sx[$max]['FollowX']} is out of range\n");
				$sx[$max] = $temp;
			}
			if (!is_int($sx[$max]))
				die("Invalid shot piece {$id}: Unknown value {$sx[$max]}\n");

			// MoveY hacks
			if (is_array($sy[$max]) && isset($sy[$max]['FollowY'])) {
				$temp = 120 + $sy[$max]['FollowY'];
				if ($temp < 100 || $temp > 255) die("Invalid shot piece {$id}: FollowY:{$sy[$max]['FollowY']} is out of range\n");
				$sy[$max] = $temp;
			}
			if (!is_int($sy[$max]))
				die("Invalid shot piece {$id}: Unknown value {$sy[$max]}\n");

			++$max;
		}
	}

	$uniques = 0;
	if ($max > 1)
	{
		if (count(array_unique($at)) == 1) $uniques |= 0x01;
		if (count(array_unique($dl)) == 1) $uniques |= 0x02;
		if (count(array_unique($bx)) == 1) $uniques |= 0x04;
		if (count(array_unique($by)) == 1) $uniques |= 0x08;
		if (count(array_unique($sg)) == 1) $uniques |= 0x10;
		if (count(array_unique($ay)) == 1) $uniques |= 0x20;
		if (count(array_unique($ax)) == 1) $uniques |= 0x40;
		if (count(array_unique($cr)) == 1) $uniques |= 0x80;
	}
	
	write_byte($output, get_default($shot, 'Delay', 0));
	write_word($output, get_default($shot, 'AnimFrames', 0));
	write_byte($output, get_default($shot, 'Homing', 0));
	write_byte($output, get_default($shot, 'PiecesPerFire', 1) | ($max << 4));
	write_byte($output, $uniques);

	$maxes = array();
	$maxes[0] = (($uniques & 0x01) ? 1 : $max);
	$maxes[1] = (($uniques & 0x02) ? 1 : $max);
	$maxes[2] = (($uniques & 0x04) ? 1 : $max);
	$maxes[3] = (($uniques & 0x08) ? 1 : $max);
	$maxes[4] = (($uniques & 0x10) ? 1 : $max);
	$maxes[5] = (($uniques & 0x20) ? 1 : $max);
	$maxes[6] = (($uniques & 0x40) ? 1 : $max);
	$maxes[7] = (($uniques & 0x80) ? 1 : $max);

	for ($j = 0; $j < $maxes[0]; ++$j) write_byte($output, $at[$j]);
	for ($j = 0; $j < $maxes[1]; ++$j) write_byte($output, $dl[$j]);
	for ($j = 0; $j < $max;      ++$j) write_byte($output, $sx[$j]);
	for ($j = 0; $j < $max;      ++$j) write_byte($output, $sy[$j]);
	for ($j = 0; $j < $maxes[2]; ++$j) write_byte($output, $bx[$j]);
	for ($j = 0; $j < $maxes[3]; ++$j) write_byte($output, $by[$j]);
	for ($j = 0; $j < $maxes[4]; ++$j) write_word($output, $sg[$j]);
	for ($j = 0; $j < $maxes[5]; ++$j) write_byte($output, $ay[$j]);
	for ($j = 0; $j < $maxes[6]; ++$j) write_byte($output, $ax[$j]);
	for ($j = 0; $j < $maxes[7]; ++$j) write_byte($output, $cr[$j]);
	write_byte($output, get_default($shot, 'Sound', 0));
	write_byte($output, get_default($shot, 'Trail', 255));
	write_byte($output, get_default($shot, 'Filter', 208));	

	write_byte($output, ord(';'));
}

printf("arcshot.dta: write OK, %d entries\n", $entries);
fclose($output);

// ----------------------------------------------------------------------------

$output = open_output('arcport.dta');
$entries = count($all_ports) - 1;
write_byte($output, $entries);

foreach ($all_ports as $id => $port)
{
	$name = sprintf("%-30.30s", get_default($port, 'Name', 'Unnamed'));
	write_byte($output, strlen($name));
	fwrite($output, $name);

	$opnum = ((isset($port['ChargeShots']) || isset($port['ChargeShots'])) ? 2 : 1);
	write_byte($output, $opnum);

	if (!isset($port['NormalShots'])) {
		for ($j = 0; $j < 11; ++$j) write_word($output, 0);
	}
	else {
		for ($j = 0; $j < 11; ++$j) {
			$temp = array_search($port['NormalShots'][$j], $shot_ids);
			if ($temp === FALSE) die("Invalid port {$id}: {$port['NormalShots'][$j]} doesn't match a shot\n");
			write_word($output, $temp);
		}
	}

	if ($opnum == 2)
	{
		if (!isset($port['ChargeShots'])) {
			for ($j = 0; $j < 5; ++$j) write_word($output, 0);
		}
		else {
			for ($j = 0; $j < 5; ++$j) {
				$temp = array_search($port['ChargeShots'][$j], $shot_ids);
				if ($temp === FALSE) die("Invalid port {$id}: {$port['ChargeShots'][$j]} doesn't match a shot\n");
				write_word($output, $temp);
			}
		}

		if (!isset($port['AimedShots'])) {
			for ($j = 0; $j < 6; ++$j) write_word($output, 0);
		}
		else {
			for ($j = 0; $j < 6; ++$j) {
				$temp = array_search($port['AimedShots'][$j], $shot_ids);
				if ($temp === FALSE) die("Invalid port {$id}: {$port['AimedShots'][$j]} doesn't match a shot\n");
				write_word($output, $temp);
			}
		}
	}

	write_word($output, get_default($port, 'Cost', 0));
	write_word($output, get_default($port, 'ItemGraphic', 1));	
	write_byte($output, ord(';'));
}

printf("arcport.dta: write OK, %d entries\n", $entries);
fclose($output);

// ----------------------------------------------------------------------------

$output = open_output('arcspec.dta');
$entries = count($all_specials) - 1;
write_byte($output, $entries);

foreach ($all_specials as $id => $special) {
	$name = sprintf("%-30.30s", get_default($special, 'Name', 'Unnamed'));
	write_byte($output, strlen($name));
	fwrite($output, $name);

	// Graphic "hacks"
	$graphic = get_default($special, "Graphic", 0);
	if ($graphic === 'Unknown') $graphic = 125;
	elseif (is_array($graphic)) {
		if (isset($graphic['ArcTyr'])) {
			$temp = $graphic['ArcTyr'];
			$graphic = $temp;
		}
		elseif (isset($graphic['Tyrian2000'])) {
			$temp = $graphic['Tyrian2000'] + 1000;
			$graphic = $temp;
		}
	}
	if (!is_int($graphic))
		die("Invalid special {$id}: Unknown value {$graphic}\n");

	// Special type list
	$stype = get_default($special, "SpecialType", 0);
	switch ($stype) {
		case 'FireOnce':              $stype = 1; break;
		case 'Repulsor':              $stype = 2; break;
		case 'Zinglon':               $stype = 3; break;
		case 'Attractor':             $stype = 4; break;
		case 'Flare':                 $stype = 5; break;
		case 'Sandstorm':             $stype = 6; break;
		case 'FlareZinglon':          $stype = 7; break;
		case 'FireMultipleRandomX':   $stype = 8; break;
		case 'FireMultiple':          $stype = 9; break;
		case 'FireMultipleLong':      $stype = 10; break;
		case 'AstralFlare':           $stype = 11; break;
		case 'InvulnerabilityShield': $stype = 12; break;
		case 'Heal':                  $stype = 13; break;
		case 'HealOtherPlayer':       $stype = 14; break;
		case 'MassSpray':             $stype = 16; break;
		case 'SpawnOption':           $stype = 17; break;
		case 'SpawnRightOption':      $stype = 18; break;
		case 'SpawnOptionAlternate':  $stype = 19; break;
		case 'Invulnerability':       $stype = 20; break;
		case 'SpawnOptionFromWeapon': $stype = 21; break;
		case 'SpawnRandomAmmoOption': $stype = 22; break;
		default:
			if (!is_int($stype))
				die("Invalid special {$id}: Unknown value {$stype}\n");
			break;
	}

	// Power types
	$power = get_default($special, "PowerUse", 0);
	if (is_array($power)) {
		if (isset($power['Shield'])) {
			if ($power['Shield'] === 'All') $power = 98;
			elseif ($power['Shield'] === 'Half') $power = 99;
			elseif ($power['Shield'] > 0 && $power['Shield'] <= 28)
				$power = $power['Shield'];
			else die("Invalid special {$id}: Shield:{$power['Shield']} is invalid\n");
		}
		elseif (isset($power['Armor'])) {
			if ($power['Armor'] > 0 && $power['Armor'] <= 28)
				$power = $power['Armor'] + 100;
			else die("Invalid special {$id}: Armor:{$power['Armor']} is invalid\n");
		}
		else die("Invalid special {$id}: Invalid PowerUse array given\n");
	}
	if (!is_int($power))
		die("Invalid special {$id}: Unknown value {$power}\n");

	// Weapon data
	$data = get_default($special, "ShotData", 0);
	if (is_array($data)) {
		if (isset($data['Shot'])) {
			$temp = array_search($data['Shot'], $shot_ids);
			if ($temp === FALSE) die("Invalid special {$id}: Shot:{$data['Shot']} doesn't match a shot\n");
			$data = $temp;
		}
		elseif (isset($data['Option'])) {
			$temp = array_search($data['Option'], $option_ids);
			if ($temp === FALSE) die("Invalid special {$id}: Option:{$data['Option']} doesn't match an option\n");
			$data = $temp;
		}
		elseif (isset($data['Side'])) {
			if ($data['Side'] === "Left") $data = 0;
			elseif ($data['Side'] === "Right") $data = 1;
			else die("Invalid special {$id}: Side:{$data['Side']} is invalid\n");
		}
		else die("Invalid special {$id}: Invalid ShotData array given\n");
	}
	if (!is_int($data))
		die("Invalid special {$id}: Unknown value {$data}\n");

	write_word($output, $graphic);
	write_byte($output, $power);
	write_byte($output, $stype);
	write_word($output, $data);

	write_byte($output, ord(';'));
}

fclose($output);
printf("arcspec.dta: write OK, %d entries\n", $entries);

// ----------------------------------------------------------------------------

$output = open_output('arcship.dta');
$entries = count($all_ships) - 1;
write_byte($output, $entries);

// Ship Select order
$entries = count($information["ShipOrder"]);
write_byte($output, $entries);
foreach ($information["ShipOrder"] as $shipname) {
	$flags = 0;
	if (is_array($shipname)) {
		if (isset($shipname['Tyrian2000'])) {
			$shipname = $shipname['Tyrian2000'];
			$flags = 0x40;
		}
		elseif (isset($shipname['Secret'])) {
			$shipname = $shipname['Secret'];
			$flags = 0x80;
		}
		else die("Invalid shiporder: Invalid array given\n");
	}

	$temp = array_search($shipname, $ship_ids);
	if ($temp === FALSE) die("Invalid shiporder: {$shipname} doesn't match a ship\n");
	write_byte($output, $temp | $flags);
}
write_byte($output, ord(';'));

foreach ($all_ships as $id => $ship) {
	$name = sprintf("%-30.30s", get_default($ship, 'Name', 'Unnamed'));
	write_byte($output, strlen($name));
	fwrite($output, $name);

	// Graphic "hacks"
	$graphic = get_default($ship, "Graphic", 0);
	if ($graphic === 'Nortship') $graphic = 1;
	elseif ($graphic === 'Dragonwing') $graphic = 0;
	elseif (is_array($graphic)) {
		if (isset($graphic['ArcTyr'])) {
			$temp = $graphic['ArcTyr'];
			$graphic = $temp;
		}
		elseif (isset($graphic['Tyrian2000'])) {
			$temp = $graphic['Tyrian2000'] + 1000;
			$graphic = $temp;
		}
	}
	if (!is_int($graphic))
		die("Invalid ship {$id}: Unknown value {$graphic}\n");

	write_word($output, $graphic);
	write_word($output, get_default($ship, 'ItemGraphic', 277)); // Unused?
	write_byte($output, get_default($ship, 'Animation', 2)); // Unused
	write_byte($output, get_default($ship, 'Speed', 0)); // Unused
	write_byte($output, get_default($ship, 'Armor', 1));
	write_word($output, get_default($ship, 'Cost', 0));
	write_byte($output, get_default($ship, 'BigShipGraphic', 32));

	if (!isset($ship['SpecialWeapons'])) {
		for ($j = 0; $j < 2; ++$j) write_word($output, 0);
	}
	else {
		for ($j = 0; $j < 2; ++$j) {
			$temp = array_search($ship['SpecialWeapons'][$j], $special_ids);
			if ($temp === FALSE) die("Invalid ship {$id}: {$ship['SpecialWeapons'][$j]} doesn't match a special\n");
			write_word($output, $temp);
		}
	}

	if (!isset($ship['PortWeapons'])) {
		for ($j = 0; $j < 5; ++$j) write_word($output, 0);
	}
	else {
		for ($j = 0; $j < 5; ++$j) {
			$temp = array_search($ship['PortWeapons'][$j], $port_ids);
			if ($temp === FALSE) die("Invalid ship {$id}: {$ship['PortWeapons'][$j]} doesn't match a port weapon\n");
			write_word($output, $temp);
		}
	}

	$twiddles = [
		[0, 0, 0, 0, 0, 0, 0, 0],
		[0, 0, 0, 0, 0, 0, 0, 0],
		[0, 0, 0, 0, 0, 0, 0, 0],
		[0, 0, 0, 0, 0, 0, 0, 0],
		[0, 0, 0, 0, 0, 0, 0, 0]
	];
	if (isset($ship['Twiddles']))
	{
		$numtwiddles = count($ship['Twiddles']);
		foreach ($ship['Twiddles'] as $twiddleNum => $twiddle) {
			foreach ($twiddle as $commandNum => $command) {
				if (is_array($command) && isset($command['Special'])) {
					$temp = array_search($command['Special'], $special_ids);
					if ($temp === FALSE) die("Invalid ship {$id}: {$command['Special']} doesn't match a special\n");
					$twiddles[$twiddleNum][$commandNum] = $temp + 100;
				}
				else switch ($command) {
					case 'Up':             $twiddles[$twiddleNum][$commandNum] = 1; break;
					case 'Down':           $twiddles[$twiddleNum][$commandNum] = 2; break;
					case 'Left':           $twiddles[$twiddleNum][$commandNum] = 3; break;
					case 'Right':          $twiddles[$twiddleNum][$commandNum] = 4; break;
					case 'Up+Fire':        $twiddles[$twiddleNum][$commandNum] = 5; break;
					case 'Down+Fire':      $twiddles[$twiddleNum][$commandNum] = 6; break;
					case 'Left+Fire':      $twiddles[$twiddleNum][$commandNum] = 7; break;
					case 'Right+Fire':     $twiddles[$twiddleNum][$commandNum] = 8; break;
					case 'Up+Sidekick':    $twiddles[$twiddleNum][$commandNum] = 9; break;
					case 'Down+Sidekick':  $twiddles[$twiddleNum][$commandNum] = 10; break;
					case 'Left+Sidekick':  $twiddles[$twiddleNum][$commandNum] = 11; break;
					case 'Right+Sidekick': $twiddles[$twiddleNum][$commandNum] = 12; break;
					case 'Release':        $twiddles[$twiddleNum][$commandNum] = 99; break;
				}
			}
		}
	}
	else
		$numtwiddles = 0;

	write_byte($output, $numtwiddles);
	for ($twnum = 0; $twnum < 5; ++$twnum)
		for ($j = 0; $j < 8; ++$j) write_byte($output, $twiddles[$twnum][$j]);

	write_byte($output, ord(';'));
}

fclose($output);
printf("arcship.dta: write OK, %d entries\n", $entries);

// ----------------------------------------------------------------------------

$output = open_output('arcopt.dta');
$entries = count($all_options) - 1;
write_byte($output, $entries);

foreach ($all_options as $id => $option) {
	$name = sprintf("%-30.30s", get_default($option, 'Name', 'Unnamed'));
	write_byte($output, strlen($name));
	fwrite($output, $name);

	$graphics = array();
	$countgr = 0;
	$animtype = 0;
	if (!isset($option['Graphics'])) {}
	elseif (is_array($option['Graphics'])) {
		$animtype = 1;
		foreach ($option['Graphics'] as $gid => $gr) {
			if (is_array($gr)) {
				if ($gid != 0) die("Invalid option {$id}: Graphics can only have an object as the first member\n");
				$gr = $gr['WaitUntilFire'];
				$animtype = 2;
			}
			$graphics[$countgr++] = $gr;
		}
	}
	elseif (is_int($option['Graphics'])) {
		$animtype = 1;
		$graphics[$countgr++] = $option['Graphics'];
	}
	else die("Invalid option {$id}: Unknown value {$graphic}\n");

	// Weapon / Charge Stages
	$chargestages = 0;
	$shottype = 0;
	if (!isset($option['Shot'])) {}
	elseif (is_array($option['Shot'])) {
		foreach ($option['Shot'] as $sid => $shot) {
			$temp = array_search($shot, $shot_ids);
			if ($temp === FALSE) die("Invalid option {$id}: {$shot} doesn't match a shot\n");

			if ($sid == 0) {
				$shottype = $temp;
				$nextsid = $temp;
			}
			elseif ($temp != $nextsid)
				die("Invalid option {$id}: Shot types are not contiguous\n");
			else
				++$chargestages;
			++$nextsid;
		}
	}
	elseif (!is_int($option['Shot'])) {
		$temp = array_search($option['Shot'], $shot_ids);
		if ($temp === FALSE) die("Invalid option {$id}: {$option['Shot']} doesn't match a shot\n");
		$shottype = $temp;
	}

	// Movement Types
	$movementtype = get_default($option, 'MovementType', 0);
	switch ($movementtype) {
		case 'Static':        $movementtype = 0; break;
		case 'BigFollower':   $movementtype = 1; break;
		case 'Front':         $movementtype = 2; break;
		case 'SmallFollower': $movementtype = 3; break;
		case 'Orbit':         $movementtype = 4; break;
		default:
			if (!is_int($movementtype))
				die("Invalid special {$id}: Unknown value {$movementtype}\n");
			break;
	}

	write_word($output, get_default($option, 'ItemGraphic', 0));
	write_word($output, get_default($option, 'Cost', 0));
	write_byte($output, $chargestages);
	write_byte($output, $movementtype | ($animtype << 4));
	write_byte($output, $countgr);
	for ($j = 0; $j < $countgr; ++$j) write_word($output, $graphics[$j]);
	write_word($output, $shottype);
	write_byte($output, get_default($option, 'Ammo', 0));
	write_byte($output, get_default($option, 'HUDIcon', 0));

	write_byte($output, ord(';'));
}

fclose($output);
printf("arcopt.dta: write OK, %d entries\n", $entries);
printf("All files written successfully!\n");

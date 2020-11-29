import argparse
import collections
import json
import os
import re



#
# Read and merge data files
#

class TyrianData:
	Shots = collections.OrderedDict({'__None__': {}})
	Ships = collections.OrderedDict({'__None__': {'Name': 'None'}})
	Ports = collections.OrderedDict({'__None__': {'Name': 'None'}})
	Options = collections.OrderedDict({'__None__': {'Name': 'None'}})
	Specials = collections.OrderedDict({'__None__': {'Name': 'None'}})
	Enemies = collections.OrderedDict()
	Info = collections.OrderedDict()

def parse_shots(data):
	TyrianData.Shots.update(data)
	print(" * %d shot%s loaded." % (len(data), "" if len(data) == 1 else "s"))

def parse_ships(data):
	TyrianData.Ships.update(data)
	print(" * %d ship%s loaded." % (len(data), "" if len(data) == 1 else "s"))

def parse_ports(data):
	TyrianData.Ports.update(data)
	print(" * %d port%s loaded." % (len(data), "" if len(data) == 1 else "s"))

def parse_options(data):
	TyrianData.Options.update(data)
	print(" * %d option%s loaded." % (len(data), "" if len(data) == 1 else "s"))

def parse_specials(data):
	TyrianData.Specials.update(data)
	print(" * %d special%s loaded." % (len(data), "" if len(data) == 1 else "s"))

def parse_enemies(data):
	TyrianData.Enemies.update(data)
	print(" * %d enem%s loaded." % (len(data), "y" if len(data) == 1 else "ies"))

def parse_info(data):
	TyrianData.Info.update(data)

def parse_import(data):
	global files, files_unparsed
	diffdata = [f for f in data if f not in files]
	files.extend(diffdata)
	files_unparsed = files_unparsed + diffdata
	print(" * %d additional file%s to load." % (len(diffdata), "" if len(diffdata) == 1 else "s"))

def parse_output(data):
	global output_path
	if output_path is not None:
		return
	print(" * Output directory is now", data)
	output_path = data

parsing = {"Shots": parse_shots,
           "Ships": parse_ships,
           "Ports": parse_ports,
           "Options": parse_options,
           "Specials": parse_specials,
           "Enemies": parse_enemies,
           "Info": parse_info,
           "Import": parse_import,
           "Output": parse_output}

def read_json(filename):
	print("Now reading", filename, "...")

	# Before parsing, remove comments
	fileptr = open(filename);
	jsondata = re.sub(r'//.*\n', '', fileptr.read())
	fileptr.close()

	data = json.loads(jsondata, object_pairs_hook=collections.OrderedDict)
	for objtype in data:
		parsing[objtype](data[objtype]);



#
# Helper functions and classes for turning json into appropriate data
#

_currentStruct = ''
_lastComplexElement = ''
class ElementError(Exception):
	def __init__(self, message):
		message += "\n... In the structure for '%s'" % _currentStruct
		super().__init__(message)

class ComplexElementError(Exception):
	def __init__(self, message):
		message += "\n... In the structure for '%s'" % _currentStruct

		# Turn ordered dicts back into regular ones so they print better on error
		if type(_lastComplexElement) is collections.OrderedDict:
			message += "\n... While resolving '%s'" % dict(_lastComplexElement)
		else:
			message += "\n... While resolving '%s'" % _lastComplexElement
		super().__init__(message)

def resolve_complex_element(elem, conversion):
	global _lastComplexElement
	_lastComplexElement = elem

	if type(elem) is int:
		return elem
	elif type(elem) is str:
		if elem in conversion and not callable(conversion[elem]):
			return conversion[elem]
		raise ComplexElementError("Unrecognized complex element '%s'" % elem)
	elif type(elem) is collections.OrderedDict:
		ekey = list(elem.keys())[0]
		if ekey in conversion and callable(conversion[ekey]):
			return conversion[ekey](elem[ekey])
		raise ComplexElementError("Unrecognized complex element '%s'" % ekey)

	raise ComplexElementError('Unexpected type received')

def invalid(reason):
	raise ComplexElementError('Invalid argument: %s' % reason)

def canonicalize_list(structure, length, function = None):
	if type(structure) is not list:
		raise ElementError('Element has the wrong type (expected list, got %s)' % type(structure))
	if len(structure) is not length:
		raise ElementError('Element has the wrong length (expected %d, got %d)' % (length, len(structure)))
	return [function(i) for i in structure] if function is not None else structure

def ensure_list_length(target, length):
	temp = target.copy()
	temp.extend([0] * length)
	return temp[:length]

class ByteString:
	def __init__(self):
		self.__s = bytearray()

	def AppendString(self, s, length):
		if len(s) > length:
			raise ElementError('String too long')
		s = ("%%-%d.%ds" % (length, length)) % s
		self.__s.append(len(s))
		self.__s.extend([ord(i) for i in s])

	def AppendUint8(self, x):
		try:
			self.__s.extend(x.to_bytes(1, signed=False, byteorder='little'))
		except Exception as e:
			raise ElementError('%s while appending bytes' % e.__class__)

	def AppendUint16(self, x):
		try:
			self.__s.extend(x.to_bytes(2, signed=False, byteorder='little'))
		except Exception as e:
			raise ElementError('%s while appending bytes' % e.__class__)

	def AppendUint32(self, x):
		try:
			self.__s.extend(x.to_bytes(4, signed=False, byteorder='little'))
		except Exception as e:
			raise ElementError('%s while appending bytes' % e.__class__)

	def AppendSint8(self, x):
		try:
			self.__s.extend(x.to_bytes(1, signed=True, byteorder='little'))
		except Exception as e:
			raise ElementError('%s while appending bytes' % e.__class__)

	def AppendSint16(self, x):
		try:
			self.__s.extend(x.to_bytes(2, signed=True, byteorder='little'))
		except Exception as e:
			raise ElementError('%s while appending bytes' % e.__class__)

	def AppendSint32(self, x):
		try:
			self.__s.extend(x.to_bytes(4, signed=True, byteorder='little'))
		except Exception as e:
			raise ElementError('%s while appending bytes' % e.__class__)

	def __bytes__(self):
		return bytes(self.__s)



#
# Resolving references to other data structures by name
#

def find_shot_id(name, is_complex = False):
	if not hasattr(find_shot_id, '_list'):
		find_shot_id._list = {}
		i = 0
		for key in TyrianData.Shots:
			find_shot_id._list[key] = i
			i += 1

	if type(name) is int:
		return name # Assume an int is a valid reference
	if name not in TyrianData.Shots.keys():
		if is_complex:
			invalid("%s doesn't match another shot" % name)
		else:
			raise ElementError("%s doesn't match another shot" % name)
	return find_shot_id._list[name]

def find_ship_id(name, is_complex = False):
	if not hasattr(find_ship_id, '_list'):
		find_ship_id._list = {}
		i = 0
		for key in TyrianData.Ships:
			find_ship_id._list[key] = i
			i += 1

	if type(name) is int:
		return name # Assume an int is a valid reference
	if name not in TyrianData.Ships.keys():
		if is_complex:
			invalid("%s doesn't match another ship" % name)
		else:
			raise ElementError("%s doesn't match another ship" % name)
	return find_ship_id._list[name]

def find_port_id(name, is_complex = False):
	if not hasattr(find_port_id, '_list'):
		find_port_id._list = {}
		i = 0
		for key in TyrianData.Ports:
			find_port_id._list[key] = i
			i += 1

	if type(name) is int:
		return name # Assume an int is a valid reference
	if name not in TyrianData.Ports.keys():
		if is_complex:
			invalid("%s doesn't match another port" % name)
		else:
			raise ElementError("%s doesn't match another port" % name)
	return find_port_id._list[name]

def find_special_id(name, is_complex = False):
	if not hasattr(find_special_id, '_list'):
		find_special_id._list = {}
		i = 0
		for key in TyrianData.Specials:
			find_special_id._list[key] = i
			i += 1

	if type(name) is int:
		return name # Assume an int is a valid reference
	if name not in TyrianData.Specials.keys():
		if is_complex:
			invalid("%s doesn't match another special" % name)
		else:
			raise ElementError("%s doesn't match another special" % name)
	return find_special_id._list[name]

def find_option_id(name, is_complex = False):
	if not hasattr(find_option_id, '_list'):
		find_option_id._list = {}
		i = 0
		for key in TyrianData.Options:
			find_option_id._list[key] = i
			i += 1

	if type(name) is int:
		return name # Assume an int is a valid reference
	if name not in TyrianData.Options.keys():
		if is_complex:
			invalid("%s doesn't match another option" % name)
		else:
			raise ElementError("%s doesn't match another option" % name)
	return find_option_id._list[name]



#
# Iterate through merged structures, return byte structures
#

def iter_shots():
	global _currentStruct
	defaults = {'Delay': 0, 'AnimFrames': 0, 'Homing': 0, 'PiecesPerFire': 1,
	            'Sound': 0, 'Trail': 255, 'Filter': 208, 'Pieces': {}}
	defaults_piece = {'AttackPower': 0, 'Duration': 255, 'MoveX': 0, 'MoveY': 0,
	                  'X': 0, 'Y': 0, 'Sprite': 0, 'AccelY': 0, 'AccelX': 0,
	                  'CircleParams': 0}
	defaults_apwr = {'Damage': 0, 'Ice': 0, 'Shrapnel': 0, 'Infinite': False}

	yield (len(TyrianData.Shots) - 1).to_bytes(2, signed=False, byteorder='little')

	for shot_id in TyrianData.Shots:
		_currentStruct = 'Shots:%s' % shot_id
		shot = {**defaults, **TyrianData.Shots[shot_id]}

		# Pieces are their own structure inside of the shot structure
		pieces = []
		for piece in shot['Pieces']:
			piece = {**defaults_piece, **piece}

			# AttackPower can be a structure of its own (oof!)
			if type(piece['AttackPower']) is collections.OrderedDict:
				apwr = {**defaults_apwr, **piece['AttackPower']}
				if int(apwr['Damage'] * 100).bit_length() > 14 or int(apwr['Ice']).bit_length() > 4:
					raise ElementError('Out of range')

				piece['AttackPower'] = int(apwr['Damage'] * 100) \
					| (apwr['Ice'] << 14) \
					| (find_shot_id(apwr['Shrapnel']) << 18) \
					| (0x80000000 if apwr['Infinite'] else 0)
			else:
				if int(piece['AttackPower'] * 100).bit_length() > 14:
					raise ElementError('Out of range')
				piece['AttackPower'] = int(piece['AttackPower'] * 100)

			# Resolve complex piece info
			piece['Duration'] = resolve_complex_element(piece['Duration'], {
				'NoTrail': 121,
				'Carry': lambda x: {'X': 98, 'Y': 100, 'XY': 99}[x],
				'SetAni': lambda x: 100 + x if 1 <= x <= 20 else invalid('Out of range') })
			piece['Sprite'] = resolve_complex_element(piece['Sprite'], {
				'Special': lambda x: 60000 + x,
				'ArcTyr': lambda x: x,
				'Tyrian2000': lambda x: x })
			piece['MoveX'] = resolve_complex_element(piece['MoveX'], {
				'FollowShip': 101,
				'FollowX': lambda x: 120 + x if -7 <= x <= 7 else invalid('Out of range') })
			piece['MoveY'] = resolve_complex_element(piece['MoveY'], {
				'FollowY': lambda x: 120 + x if -7 <= x <= 7 else invalid('Out of range') })

			pieces.append(piece)

		s = ByteString()
		s.AppendUint8(shot['Delay'])
		s.AppendUint16(shot['AnimFrames'])
		s.AppendUint8(shot['Homing'])
		s.AppendUint8(shot['PiecesPerFire'] | len(shot['Pieces']) << 4)
		[s.AppendUint32(i) for i in [p['AttackPower'] for p in pieces]]
		[s.AppendUint8(i) for i in [p['Duration'] for p in pieces]]
		[s.AppendSint8(i) for i in [p['MoveX'] for p in pieces]]
		[s.AppendSint8(i) for i in [p['MoveY'] for p in pieces]]
		[s.AppendSint8(i) for i in [p['X'] for p in pieces]]
		[s.AppendSint8(i) for i in [p['Y'] for p in pieces]]
		[s.AppendUint16(i) for i in [p['Sprite'] for p in pieces]]
		[s.AppendSint8(i) for i in [p['AccelY'] for p in pieces]]
		[s.AppendSint8(i) for i in [p['AccelX'] for p in pieces]]
		[s.AppendUint8(i) for i in [p['CircleParams'] for p in pieces]]
		s.AppendUint8(shot['Sound'])
		s.AppendUint8(shot['Trail'])
		s.AppendUint8(shot['Filter'])
		yield s
		yield [59]
	# End iteration
	yield len(TyrianData.Shots)

def iter_ships():
	global _currentStruct
	defaults = {'Name': 'Unnamed', 'Graphic': 0, 'ItemGraphic': 277,
	            'Animation': 2, 'Speed': 0, 'Armor': 1, 'Cost': 0,
	            'BigShipGraphic': 32, 'SpecialWeapons': [0, 0],
	            'PortWeapons': [0, 0, 0, 0, 0], 'StartingOptions': {},
	            'Twiddles':[]}

	yield (len(TyrianData.Ships) - 1).to_bytes(1, signed=False, byteorder='little')

	# Ship Select order
	_currentStruct = 'Info:ShipOrder'
	shiporder = TyrianData.Info['ShipOrder']
	s = ByteString()
	s.AppendUint8(len(shiporder))
	for ship_name in shiporder:
		try:
			ship = resolve_complex_element(ship_name, {
				'Tyrian2000': lambda x: find_ship_id(x, is_complex=True) | 0x40, # Prevents T2K ships in non-T2K
				'Secret': lambda x: find_ship_id(x, is_complex=True) | 0x80 }) # Hidden, code-only
		except ComplexElementError:
			ship = find_ship_id(ship_name)
		finally:
			s.AppendUint8(ship)
	yield s
	yield [59]

	for ship_id in TyrianData.Ships:
		_currentStruct = 'Ships:%s' % ship_id
		ship = {**defaults, **TyrianData.Ships[ship_id]}

		# Resolve complex graphics element
		ship['Graphic'] = resolve_complex_element(ship['Graphic'], {
			'Nortship': 1,
			'Dragonwing': 0,
			'ArcTyr': lambda x: x,
			'Tyrian2000': lambda x: 1000 + x }) # Uses a different bank

		# Resolve 'Random' weapons
		if ship['SpecialWeapons'] == 'Random':
			ship['SpecialWeapons'] = [0xFFFF] * 2
		if ship['PortWeapons'] == 'Random':
			ship['PortWeapons'] = [0xFFFF] * 5

		# Starting options are a little weird ... artifact of the old script
		# {'Left': 'OptionID', 'Right': 'OptionID2'}
		# Either or both may be omitted
		starting_options = [0, 0]
		for side in ship['StartingOptions']:
			starting_options[{'Left':0, 'Right':1}[side]] = find_option_id(ship['StartingOptions'][side])

		# And twiddles are even weirder...
		# List of commands terminated by {'Special': 'SpecialID'}
		# e.g. ['Left', 'Right', 'Up+Fire', {Special: 'SpecialID'}]
		twiddles = []
		for twiddle in ship['Twiddles']:
			t = [resolve_complex_element(i, {
				'Up': 1,
				'Down': 2,
				'Left': 3,
				'Right': 4,
				'Up+Fire': 5,
				'Down+Fire': 6,
				'Left+Fire': 7,
				'Right+Fire': 8,
				'Up+Sidekick': 9,
				'Down+Sidekick': 10,
				'Left+Sidekick': 11,
				'Right+Sidekick': 12,
				'Up+Fire+Sidekick': 13,
				'Down+Fire+Sidekick': 14,
				'Left+Fire+Sidekick': 15,
				'Right+Fire+Sidekick': 16,
				'Release': 99,
				'Special': lambda x: 100 + find_special_id(x, is_complex=True) }) for i in twiddle]
			# Make 8 entries long no matter what
			twiddles.append(ensure_list_length(t, 8))

		s = ByteString()
		s.AppendString(ship['Name'], 30)
		s.AppendUint16(ship['Graphic'])
		s.AppendUint16(ship['ItemGraphic'])
		s.AppendUint8(ship['Animation'])
		s.AppendUint8(ship['Speed'])
		s.AppendUint8(ship['Armor'])
		s.AppendUint16(ship['Cost'])
		s.AppendUint8(ship['BigShipGraphic'])
		[s.AppendUint16(i) for i in canonicalize_list(ship['SpecialWeapons'], length=2, function=find_special_id)]
		[s.AppendUint16(i) for i in canonicalize_list(ship['PortWeapons'], length=5, function=find_port_id)]
		[s.AppendUint8(i) for i in starting_options]
		s.AppendUint8(len(twiddles))
		for twiddle in twiddles:
			[s.AppendUint8(i) for i in twiddle]

		yield s
		yield [59]
	# End iteration
	yield len(TyrianData.Ships)

def iter_ports():
	global _currentStruct
	defaults = {'Name': 'Unnamed', 'Cost': 0, 'ItemGraphic': 1,
	            'NormalShots': [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0],
	            'ChargeShots': [0, 0, 0, 0, 0],
	            'AimedShots': [0, 0, 0, 0, 0, 0],
	            'DragonWingOptions': [0, 0, 0]}

	yield (len(TyrianData.Ports) - 1).to_bytes(1, signed=False, byteorder='little')

	for port_id in TyrianData.Ports:
		_currentStruct = 'Ports:%s' % port_id
		port = {**defaults, **TyrianData.Ports[port_id]}

		is_dragonwing_capable = False if port['ChargeShots'][0] == 0 else True

		s = ByteString()
		s.AppendString(port['Name'], 30)
		s.AppendUint8(2 if is_dragonwing_capable else 1)
		[s.AppendUint16(i) for i in canonicalize_list(port['NormalShots'], length=11, function=find_shot_id)]
		if is_dragonwing_capable:
			[s.AppendUint16(i) for i in canonicalize_list(port['ChargeShots'], length=5, function=find_shot_id)]
			[s.AppendUint16(i) for i in canonicalize_list(port['AimedShots'], length=6, function=find_shot_id)]
			[s.AppendUint8(i) for i in canonicalize_list(port['DragonWingOptions'], length=3, function=find_option_id)]
		s.AppendUint16(port['Cost'])
		s.AppendUint16(port['ItemGraphic'])
		yield s
		yield [59]
	# End iteration
	yield len(TyrianData.Ports)

def iter_specials():
	global _currentStruct
	defaults = {'Name': 'Unnamed', 'Graphic': 0, 'SpecialType': 0,
	            'PowerUse': 0, 'ShotType': 0, 'ExtraData': 0}

	yield (len(TyrianData.Specials) - 1).to_bytes(1, signed=False, byteorder='little')

	# Too complex for lambda
	def resolve_shield_use(x):
		if type(x) is str:
			if x == 'All':
				return 98
			elif x == 'Half':
				return 99
			invalid('Unsupported string option')
		if type(x) is int:
			return x if 1 <= x <= 28 else invalid('Out of range')
		invalid('Unsupported type')

	for spec_id in TyrianData.Specials:
		_currentStruct = 'Specials:%s' % spec_id
		spec = {**defaults, **TyrianData.Specials[spec_id]}

		# Almost every element of this is complex, so
		spec['Graphic'] = resolve_complex_element(spec['Graphic'], {'Unknown': 125})
		spec['PowerUse'] = resolve_complex_element(spec['PowerUse'], {
			'Shield': resolve_shield_use,
			'Armor': lambda x: 100 + x if 1 <= x <= 28 else invalid('Out of range'),
			'PowerLevel': lambda x: 200 + x if 1 <= x <= 1 else invalid('Out of range') })
		spec['ExtraData'] = resolve_complex_element(spec['ExtraData'], {
			'Option': lambda x: find_option_id(x, is_complex=True),
			'Side': lambda x: {'Left': 0, 'Right': 1}[x] })
		spec['SpecialType'] = resolve_complex_element(spec['SpecialType'], {
			'FireOnce': 1,
			'Repulsor': 2,
			'Zinglon': 3,
			'Attractor': 4,
			'Flare': 5,
			'Sandstorm': 6,
			'FlareZinglon': 7,
			'FireMultipleRandomX': 8,
			'FireMultiple': 9,
			'FireMultipleLong': 10,
			'AstralFlare': 11,
			'InvulnerabilityShield': 12,
			'Heal': 13,
			'HealOtherPlayer': 14,
			'MassSpray': 16,
			'SpawnOption': 17,
			'SpawnRightOption': 18,
			'SpawnOptionAlternate': 19,
			'Invulnerability': 20,
			'SpawnOptionFromWeapon': 21,
			'SpawnRandomAmmoOption': 22,
			'FireWhileSidekickHeld': 23,
			'FireWhileFireHeld': 24 })
		spec['ShotType'] = find_shot_id(spec['ShotType'])

		s = ByteString()
		s.AppendString(spec['Name'], 30)
		s.AppendUint16(spec['Graphic'])
		s.AppendUint8(spec['PowerUse'])
		s.AppendUint8(spec['SpecialType'])
		s.AppendUint16(spec['ShotType'])
		s.AppendUint8(spec['ExtraData'])
		yield s
		yield [59]
	# End iteration
	yield len(TyrianData.Specials)

def iter_options():
	global _currentStruct
	defaults = {'Name': 'Unnamed', 'Graphics': None, 'Shot': 0,
	            'MovementType': 0, 'ItemGraphic': 0, 'Cost': 0,
	            'Ammo': 0, 'HUDIcon': 0}

	yield (len(TyrianData.Options) - 1).to_bytes(1, signed=False, byteorder='little')

	for opt_id in TyrianData.Options:
		_currentStruct = 'Options:%s' % opt_id
		opt = {**defaults, **TyrianData.Options[opt_id]}

		# Complex movement type
		opt['MovementType'] = resolve_complex_element(opt['MovementType'], {
			'Static': 0,
			'BigFollower': 1,
			'Front': 2,
			'SmallFollower': 3,
			'Orbit': 4 })

		# Extra-complex graphic type
		# Can be nonexistant, which hides graphics
		# Can be an int, which just displays that one sprite
		# Can be a list, which constantly goes through those sprites
		# Can be a list with the starting element {'WaitUntilFire': int}
		animation_type = 1
		def wait_until_fire(x):
			nonlocal animation_type
			animation_type = 2
			return x

		if opt['Graphics'] is None:
			opt['Graphics'] = []
			animation_type = 0 # Hidden
		elif type(opt['Graphics']) is not list:
			opt['Graphics'] = [opt['Graphics']]
		else:
			opt['Graphics'][0] = resolve_complex_element(opt['Graphics'][0], {'WaitUntilFire': wait_until_fire})

		# Shot can be an array, but the elements *must* be contiguous
		# e.g. ['Shot1', 'Shot2', 'Shot3'] must resolve to [1, 2, 3], etc.
		charge_stages = 0 # And this is why
		if type(opt['Shot']) is list:
			given_shots = [find_shot_id(i) for i in opt['Shot']]
			wanted_shots = [*range(given_shots[0], given_shots[0] + len(given_shots))]
			if given_shots != wanted_shots:
				raise ElementError("Shot values aren't contiguous")

			opt['Shot'] = given_shots[0]
			charge_stages = len(given_shots) - 1
		else:
			opt['Shot'] = find_shot_id(opt['Shot'])

		s = ByteString()
		s.AppendString(opt['Name'], 30)
		s.AppendUint16(opt['ItemGraphic'])
		s.AppendUint16(opt['Cost'])
		s.AppendUint8(charge_stages)
		s.AppendUint8(opt['MovementType'] | animation_type << 4)
		s.AppendUint8(len(opt['Graphics']))
		[s.AppendUint16(i) for i in opt['Graphics']]
		s.AppendUint16(opt['Shot'])
		s.AppendUint8(opt['Ammo'])
		s.AppendUint8(opt['HUDIcon'])
		yield s
		yield [59]
	# End iteration
	yield len(TyrianData.Options)

def iter_enemies():
	global _currentStruct
	defaults = {'Graphics': [0], 'Shots': [], 'ShotFrequency': [],
	            'MoveX': 0, 'MoveY': 0, 'XAccel': 0, 'YAccel': 0,
	            'XCAccel': 0, 'YCAccel': 0, 'StartX': 0, 'StartY': 0,
	            'StartXC': 0, 'StartYC': 0, 'Health': 1, 'Size': 1,
	            'ExplosionType': 30, 'Animate': 0, 'ShapeBank': 0,
	            'XRev': 0, 'YRev': 0, 'DGR': 0, 'DLevel': 0,
	            'DAni': 0, 'ELaunchFreq': 0, 'ELaunchType': 0,
	            'Value': 0, 'EEnemyDie': 0}
	presets = {
		'SwayingPowerUp': {
			'MoveX': 4,
			'MoveY': 1,
			'XCAccel': 4,
			'XRev': 4,
			'Health': 0,
			'ExplosionType': 0,
			'ShapeBank': 26,
			'Value': 30001},
		'FallingPowerUp': {
			'MoveY': 1,
			'StartX': 130,
			'StartY': -13,
			'StartXC': 125,
			'Health': 0,
			'ExplosionType': 0,
			'ShapeBank': 21,
		'Value': 20001}
	}

	yield [100] # Fixed number of entries (Enemies must fit in slots 900-999)
	for i in range(900, 1000):
		enemy_id = 'Slot%d' % i
		_currentStruct = 'Options:%s' % enemy_id
		if enemy_id in TyrianData.Enemies:
			enemy = TyrianData.Enemies[enemy_id]
			if 'Preset' in enemy:
				enemy = {**presets[enemy['Preset']], **enemy}
			enemy = {**defaults, **enemy}
		else:
			enemy = {**defaults}

		# Complex value field
		enemy['Value'] = resolve_complex_element(enemy['Value'], {
			'GiveWeapon': lambda x: 11000 + find_port_id(x),
			'GiveOption': lambda x: 12000 + find_option_id(x),
			'GiveSpecial': lambda x: 13000 + find_special_id(x),
			'Armor': lambda x: 20000 + x })

		# Extend shot stuff to (at least) 3 elements, graphics 20
		num_graphics = len(enemy['Graphics'])
		enemy['Shots'] = ensure_list_length(enemy['Shots'], 3)
		enemy['ShotFrequency'] = ensure_list_length(enemy['ShotFrequency'], 3)
		enemy['Graphics'] = ensure_list_length(enemy['Graphics'], 20)

		s = ByteString()
		s.AppendUint8(num_graphics)
		[s.AppendUint8(i) for i in enemy['Shots']]
		[s.AppendUint8(i) for i in enemy['ShotFrequency']]
		s.AppendSint8(enemy['MoveX'])
		s.AppendSint8(enemy['MoveY'])
		s.AppendSint8(enemy['XAccel'])
		s.AppendSint8(enemy['YAccel'])
		s.AppendSint8(enemy['XCAccel'])
		s.AppendSint8(enemy['YCAccel'])
		s.AppendSint16(enemy['StartX'])
		s.AppendSint16(enemy['StartY'])
		s.AppendSint8(enemy['StartXC'])
		s.AppendSint8(enemy['StartYC'])
		s.AppendUint8(enemy['Health'])
		s.AppendUint8(enemy['Size'])
		[s.AppendUint16(i) for i in enemy['Graphics']]
		s.AppendUint8(enemy['ExplosionType'])
		s.AppendUint8(enemy['Animate'])
		s.AppendUint8(enemy['ShapeBank'])
		s.AppendSint8(enemy['XRev'])
		s.AppendSint8(enemy['YRev'])
		s.AppendUint16(enemy['DGR'])
		s.AppendSint8(enemy['DLevel'])
		s.AppendSint8(enemy['DAni'])
		s.AppendUint8(enemy['ELaunchFreq'])
		s.AppendUint16(enemy['ELaunchType'])
		s.AppendSint16(enemy['Value'])
		s.AppendUint16(enemy['EEnemyDie'])
		yield s
		yield [59]
	# End iteration
	yield len(TyrianData.Enemies)

def write_iterator(filename, iteration):
	count = 0
	f = open(os.path.join(output_path, filename), 'wb')
	for i in iteration:
		if type(i) is int:
			print('%s: write successful (wrote %d entries)' % (filename, i))
			break
		f.write(bytes(i))
	f.close()



#
# Main program
#

argp = argparse.ArgumentParser(
	description="Merge and convert json data files to ArcTyr format.")
argp.add_argument('file', nargs='+', help="File(s) to start parsing")
argp.add_argument('--output', default=None, help="Output folder for new files")
arguments = argp.parse_args()

files = arguments.file
files_unparsed = arguments.file
output_path = arguments.output

# This list is modified by read_json,
# so iterating over it normally is out of the question
while len(files_unparsed) > 0:
	read_json(files_unparsed.pop(0))

output_path = os.path.normpath(output_path)

write_iterator('arcshot.dta', iter_shots())
write_iterator('arcship.dta', iter_ships())
write_iterator('arcport.dta', iter_ports())
write_iterator('arcspec.dta', iter_specials())
write_iterator('arcopt.dta', iter_options())
write_iterator('enemies.dta', iter_enemies())

print('...OK')

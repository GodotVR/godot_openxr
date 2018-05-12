extends Spatial

var openhmd_config

func _ready():
	# get our configuration object
	openhmd_config = preload("res://addons/godot-openhmd/OpenHMDConfig.gdns").new()

	# we'll eventually be able to disable using the first device automatically
	
	# and then just find the interface
	var arvr_interface = ARVRServer.find_interface("OpenHMD")
	if arvr_interface and arvr_interface.initialize():
		# list devices
		print("Listing devices:")
		var list_devices = openhmd_config.list_devices()
		for device in list_devices:
			print("Device " + str(device['device_no']) + " " + device['vendor'] + " - " + device['product'])
		
		# for now we hardcode
#		openhmd_config.init_hmd_device(0)
#		openhmd_config.init_tracking_device(1)
#		openhmd_config.init_controller_device(2)
#		openhmd_config.init_controller_device(3)

		# check oversample
		print("Oversample was: " + str(openhmd_config.get_oversample()))
		openhmd_config.set_oversample(1.5)
		
		# and tell our viewport to render
		get_viewport().arvr = true
		


func _process(delta):
	# Test for escape to close application, space to reset our reference frame
	if (Input.is_key_pressed(KEY_ESCAPE)):
		get_tree().quit()
	elif (Input.is_key_pressed(KEY_SPACE)):
		# Calling center_on_hmd will cause the ARVRServer to adjust all tracking data so the player is centered on the origin point looking forward
		ARVRServer.center_on_hmd(true, true)

	# We minipulate our origin point to move around. Note that with roomscale tracking a little more then this is needed
	# because we'll rotate around our origin point, not around our player. But that is a subject for another day.
	if (Input.is_key_pressed(KEY_LEFT)):
		$ARVROrigin.rotation.y += delta
	elif (Input.is_key_pressed(KEY_RIGHT)):
		$ARVROrigin.rotation.y -= delta

	if (Input.is_key_pressed(KEY_UP)):
		$ARVROrigin.translation -= $ARVROrigin.transform.basis.z * delta;
	elif (Input.is_key_pressed(KEY_DOWN)):
		$ARVROrigin.translation += $ARVROrigin.transform.basis.z * delta;
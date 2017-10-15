extends Spatial

var openhmd_config

func _ready():
	# get our configuration object
	openhmd_config = preload("OpenHMDConfig.gdns").new()
	
	# we'll eventually be able to disable using the first device automatically
	
	# and then just find the interface
	var arvr_interface = ARVRServer.find_interface("OpenHMD")
	if arvr_interface and arvr_interface.initialize():		
		# we'll soon add the ability to list the available devices
		
		# for now we hardcode
		openhmd_config.init_hmd_device(3)
		openhmd_config.init_tracking_device(0)
		openhmd_config.init_controller_device(1)
		openhmd_config.init_controller_device(2)
		
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
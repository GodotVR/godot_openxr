extends ARVROrigin

var interface : ARVRInterface

func initialise() -> bool:
	var interface = ARVRServer.find_interface("OpenXR")
	if interface and interface.initialize():
		print("OpenXR Interface initialized")

		# Change our viewport so it is tied to our ARVR interface and renders to our HMD
		get_viewport().arvr = true

		# increase our physics engine update speed
		Engine.iterations_per_second = 144

		return true
	else:
		return false

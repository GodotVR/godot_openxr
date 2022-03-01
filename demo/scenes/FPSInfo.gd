extends Node2D


# Called every frame. 'delta' is the elapsed time since the previous frame.
func _process(delta):
	$Label.text = "FPS: " + str(Performance.get_monitor(Performance.TIME_FPS))

extends Node2D

var count = 0

# Called when the node enters the scene tree for the first time.
func _ready():
	pass # Replace with function body.

func _on_Button_pressed():
	count = count + 1
	$Count.text = "Pressed %d times" % [ count ]

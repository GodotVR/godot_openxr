extends Node2D


var score = 0

func inc_score():
	score = score + 1
	$Score.text = "Score: " + str(score)

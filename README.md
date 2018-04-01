# README #

This is the Stair Light Project, started in October 2016. It is now March 2018 and the project is nearing its first completion. The idea is to create a nice, animated and automated light using SK6812 RBGW LEDs, to make it even more enjoyable to walk up (or down) the stairs.
Each step is equipped with a bar of (in my case) 27 RGBW LEDs. The first and last step each have an IR sensor (SR-HC501) which will trigger an animation, if somebody walks the stairs.

### What is this repository for? ###

As stated: for a cool stair light

### How do I get set up? ###

This project is based on RGBW LED stripes (I am using SK6812, according to some internet sources, they are supposed to be more robust, but fully compatible to the "normal" WS2812) and an ESP8266 as a controller.


### Contribution guidelines ###

I am looking for contributors who would be willing to come up with some cool animations, whatever the ESP8266 and the SK6812 can master. It would probably be best, if you build your own test bed. I have a test bed here, with an ESP8266 on a bread board, connected to 5 "steps" (rows) of SK6812 LEDs. In order for it to be small, I used the 144LEDs/m strips to build this.

<table class="image">
<caption align="bottom">SK6812, 144 LEDs / m, IP67</caption>
<tr>
<td>
<img src="https://github.com/ThomasStolt/Stair-Light-Project/blob/master/images/IMG_1958.jpg" height="300"/>
</td>
</tr>
</table>

I am trying to come up with a sort of "API" whereby all parameters for an animation is being passed to a animation function. That way, I can keep the general framework the same and people can add animations, if they wish to.

### Who do I talk to? ###

Talk to me if you need to

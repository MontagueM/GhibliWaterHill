# UEAlyx
A small VR project to focus on tuning player movement and VR visual design.

From commit on 27th April the mechanic work is mostly completed.

I wanted to test out programming the following VR mechanics in C++:
- Controller movement
- Controller rotation
- Teleportation
- Controller grabbing

I also added some more specific parts:
- HMD post-processing
- Half-Life: Alyx "flick" grab
- Whitebox mapping

The most interesting part of my ~2 weeks on this was understanding how players want to move in VR, dealing with VR sickness, and figuring out exactly how Valve made the "flick" grab so good.

Comparison:
![hlaflick](https://i.imgur.com/ilmiidx.jpg)
![myflick](https://i.imgur.com/DqYt6PH.png)

It is clear to see that my "flick" needs work, especially with materials. However, after some small tuning with how the Bezier spline curve is calculated the grab line looks much closer to HL:A.
The actual movement and flying can sometimes lag around, but when it works it seems to be very similar in style to the game.

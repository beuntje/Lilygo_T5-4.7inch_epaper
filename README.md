# Lilygo T5 4.7 inch e-paper: Load online image

After a long online search, I found no way to display an online image on the e-paper screen of my Lilygo T5-4.7 inch e-paper.  
So here is a somewhat hacky way to load a jpg / png image to the T5.... But it works :-)

## Methodology

- ESP32 requests pieces of the picture to the php website
- Web server crops and examines the pieces and forwards the requested data
- ESP32 puts the obtained data into the buffer pixel by pixel
- After the entire image is loaded, the buffer is sent to the epaper 

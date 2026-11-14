# This lib cant play directly .mp3 files form the rom, so we need to convert a file in order to make it play...

- 1st step: convert the file into raw pcm sound using: 
```sox input.wav -t raw -b 16 --endian big -c 2 output.raw && ```
xxd -i mysound.raw | sed 's/unsigned/const unsigned/' > source/mysound_raw.c


sox layton.wav -t raw -b 16 --endian big -c 2 output.raw &&  xxd -i output.raw | sed 's/unsigned/const unsigned/' > room_3.c
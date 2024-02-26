PRJNAME := river_strike
OBJS := data.rel actor.rel map.rel score.rel status.rel river_strike.rel

all: $(PRJNAME).sms

data.c: data/* data/sprites_tiles.psgcompr data/tileset_tiles.psgcompr data/title_tiles.psgcompr \
		data/gauge_tiles.bin \
		data/engine_fast.psg data/engine_normal.psg data/engine_slow.psg data/fueling.psg data/shot.psg data/explosion.psg
	folder2c data data
	
data/sprites_tiles.psgcompr: data/img/sprites.png
	BMP2Tile.exe data/img/sprites.png -noremovedupes -8x16 -palsms -fullpalette -savetiles data/sprites_tiles.psgcompr -savepalette data/sprites_palette.bin
	
data/gauge_tiles.bin: data/img/gauge.png
	BMP2Tile.exe data/img/gauge.png -noremovedupes -8x16 -palsms -fullpalette -savetiles data/gauge_tiles.bin

data/tileset_tiles.psgcompr: data/img/tileset.png
	BMP2Tile.exe data/img/tileset.png -noremovedupes -8x16 -palsms -fullpalette -savetiles data/tileset_tiles.psgcompr -savepalette data/tileset_palette.bin

data/title_tiles.psgcompr: data/img/title.png
	BMP2Tile.exe data/img/title.png -palsms -fullpalette -savetiles data/title_tiles.psgcompr -savetilemap data/title_tilemap.bin -savepalette data/title_palette.bin

data/%.path: data/path/%.spline.json
	node tool/convert_splines.js $< $@

data/%.bin: data/map/%.tmx
	node tool/convert_map.js $< $@

data/%.psg: data/deflemask/%.vgm
	vgm2psg $< $@
	
%.vgm: %.wav
	psgtalk -r 512 -u 1 -m vgm $<

%.rel : %.c
	sdcc -c -mz80 --peep-file lib/peep-rules.txt $<

$(PRJNAME).sms: $(OBJS)
	sdcc -o $(PRJNAME).ihx -mz80 --no-std-crt0 --data-loc 0xC000 lib/crt0_sms.rel $(OBJS) SMSlib.lib lib/PSGlib.rel
	ihx2sms $(PRJNAME).ihx $(PRJNAME).sms	

clean:
	rm *.sms *.sav *.asm *.sym *.rel *.noi *.map *.lst *.lk *.ihx data.*

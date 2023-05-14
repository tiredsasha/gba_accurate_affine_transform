#include <tonc.h>
//needed for the memcpy function
#include <string.h>

//the image of a kittycat
#include "kittycat.h"

//floats aren't ideal on the gba, but they're needed here to do software maths for sx and sy
//in this example we don't need absolute performance, so floats and division etc are fine
float aff_x1 =  70;
float aff_y1 =  30;
float aff_x2 = 170;
float aff_y2 = 130;

//the width and height based on the previous values
float aff_width;
float aff_height;
//the stretched x and y values that we'll plug into memory
int aff_stretch_x;
int aff_stretch_y;

//this is for OAM double buffering
OBJ_ATTR obj_buffer[128];
OBJ_AFFINE *obj_aff_buffer = (OBJ_AFFINE*)obj_buffer;

//the sprite itself
OBJ_ATTR *sillykitty;
//the affine transform that we'll attach to the sprite
OBJ_AFFINE *sillykitty_transform;

//what's the original sprite width and height
#define stretch_numerator1 64
//what's the gba's value for "1" here. or: how many texels are in one pixel
#define stretch_numerator2 256
//what's tile does the kittycat start on
#define tile_start 512

//what's the width and height of the sprite
#define sprite_width  128
#define sprite_height 128

int main(void) {
	//load in the tiles of the kittycat
	memcpy(&tile_mem[4][tile_start], kittycatTiles, kittycatTilesLen);
	//load in the palette of the kittycat
	memcpy(pal_obj_mem, kittycatPal, kittycatPalLen);
	
	//display control, we turn things on and off here that we want
	//DCNT_MODE4 is mode 4, which is fullscreen and has page flipping
	//DCNT_BG2 enables rendering of background 2 (which is what the bitmap displays on)
	//DCNT_OBJ enables object rendering, for sprites
	//DCNT_OBJ_1D sets it so tiles are rendered from 1d mode
	REG_DISPCNT = DCNT_MODE4 | DCNT_BG2 | DCNT_OBJ | DCNT_OBJ_1D;
	//hide any sprites already on screen
	oam_init(obj_buffer,128);
	
	//add two colors to the background palette, for the bitmap
	pal_bg_mem[1] = 0b0000110001111111; //red
	pal_bg_mem[2] = 0b0000111111100011; //green
	
	//enable vsync
	irq_init(NULL);
	irq_enable(II_VBLANK);
	
	//attach the OAM object to the buffer
	sillykitty = &obj_buffer[0];
	//set up the OAM object
	obj_set_attr(sillykitty,
			ATTR0_SQUARE | ATTR0_8BPP | ATTR0_AFF | ATTR0_AFF_DBL, //square, 8bpp, affine, double size
			ATTR1_SIZE_64x64 | ATTR1_AFF_ID(0),                    //64x64p, affine index 0
			ATTR2_ID(tile_start));                                 //the VRAM tile we're pointing to
	//attach the affine transform to the buffer. we'll modify the transform later
	sillykitty_transform = &obj_aff_buffer[0];
	
	while (1) {
		//reset the bitmap
		m4_fill(0);
		
		//grab the keys
		key_poll();
		
		//modify the bounding box based on input
		if (key_is_down(KEY_A | KEY_B | KEY_START | KEY_SELECT)) {
			aff_x2 += key_tri_horz();
			aff_y2 += key_tri_vert();
		} else {
			aff_x1 += key_tri_horz();
			aff_y1 += key_tri_vert();
		}
		
		//keep this within screen bounds
		if (aff_x1 < 1) {
			aff_x1 = 1;
		} else if (aff_x1 > 238) {
			aff_x1 = 238;
		}
		if (aff_x2 < 1) {
			aff_x2 = 1;
		} else if (aff_x2 > 238) {
			aff_x2 = 238;
		}
		if (aff_y1 < 1) {
			aff_y1 = 1;
		} else if (aff_y1 > 158) {
			aff_y1 = 158;
		}
		if (aff_y2 < 1) {
			aff_y2 = 1;
		} else if (aff_y2 > 158) {
			aff_y2 = 158;
		}
		
		//calculate width and height
		//our width and height are inclusive to x2 and y2, so we add 1 to account for that
		aff_width  = aff_x2 - aff_x1 + 1;
		aff_height = aff_y2 - aff_y1 + 1;
		
		//calculate the stretch x and y values to use for the affine transform
		//we add 0.5 so that it's rounded to the nearest integer
		aff_stretch_x = (int)((stretch_numerator1 * stretch_numerator2 / aff_width) + 0.5);
		aff_stretch_y = (int)((stretch_numerator1 * stretch_numerator2 / aff_height) + 0.5);
		
		//now apply this to the affine transform
		obj_aff_rotscale(sillykitty_transform,aff_stretch_x,aff_stretch_y,0);
		
		//position the OAM object
		//i know it would make sense to round it, but it's more accurate to the top and left if it's floored
		obj_set_pos(sillykitty,
					(int)(aff_x1 + aff_width/2 - sprite_width/2),
					(int)(aff_y1 + aff_height/2 - sprite_height/2));
		
		//draw a red rectangle on the bitmap to show what we _want_ to display
		m4_frame((int)aff_x1,(int)aff_y1,(int)aff_x2+1,(int)aff_y2+1, 1);
		//draw a green rectangle around the previous one to make sure the sprite hasn't gone too far
		m4_frame((int)aff_x1-1,(int)aff_y1-1,(int)aff_x2+2,(int)aff_y2+2, 2);
		
		//vsync
		VBlankIntrWait();
		//update oam
		oam_copy(oam_mem, obj_buffer, 128);
		//flip the page
		vid_flip();
	}
}

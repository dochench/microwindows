/*
 * Author: Tony Rogvall <tony@bluetail.com>
 *
 */
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <X11/Xlib.h>
#include "device.h"

#define	SCALE		3	/* default scaling factor for acceleration */
#define	THRESH		5	/* default threshhold for acceleration */

static int  	X11_Open(MOUSEDEVICE *pmd);
static void 	X11_Close(void);
static int  	X11_GetButtonInfo(void);
static void	X11_GetDefaultAccel(int *pscale,int *pthresh);
static int  	X11_Read(MWCOORD *dx, MWCOORD *dy, MWCOORD *dz, int *bp);

extern Display*     x11_dpy;
extern int          x11_scr;
extern Visual*      x11_vis;
extern Window       x11_win;
extern GC           x11_gc;
extern unsigned int x11_event_mask;
int          x11_setup_display(void);
void         x11_handle_event(XEvent*);

MOUSEDEVICE mousedev = {
    X11_Open,
    X11_Close,
    X11_GetButtonInfo,
    X11_GetDefaultAccel,
    X11_Read,
    NULL,
    MOUSE_NORMAL	/* flags*/
};

/*
 * Open up the mouse device.
 * Returns the fd if successful, or negative if unsuccessful.
 */
static int X11_Open(MOUSEDEVICE *pmd)
{
    if (x11_setup_display() < 0)
		return DRIVER_FAIL;
    /* return the x11 file descriptor for select */
    return DRIVER_OKFILEDESC(ConnectionNumber(x11_dpy));
}

/*
 * Close the mouse device.
 */
static void
X11_Close(void)
{
    /* nop */
}

/*
 * Get mouse buttons supported
 */
static int
X11_GetButtonInfo(void)
{
	return MWBUTTON_L | MWBUTTON_M | MWBUTTON_R;
}

/*
 * Get default mouse acceleration settings
 */
static void
X11_GetDefaultAccel(int *pscale,int *pthresh)
{
    *pscale = SCALE;
    *pthresh = THRESH;
}

/*
 * Attempt to read bytes from the mouse and interpret them.
 * Returns -1 on error, 0 if either no bytes were read or not enough
 * was read for a complete state, or 1 if the new state was read.
 * When a new state is read, the current buttons and x and y deltas
 * are returned.  This routine does not block.
 */
static int
X11_Read(MWCOORD *dx, MWCOORD *dy, MWCOORD *dz, int *bp)
{
    XEvent ev;
    int events = 0;
    long mask = x11_event_mask | ButtonPressMask | ButtonReleaseMask | PointerMotionMask;

    while (XCheckMaskEvent(x11_dpy, mask, &ev)) {
	if (ev.type == MotionNotify) {
	    if (ev.xmotion.window == x11_win) {
		int button = 0;
		*dx = ev.xmotion.x;
		*dy = ev.xmotion.y;
		*dz = 0;
		if (ev.xmotion.state & Button1Mask)
		    button |= MWBUTTON_L;
		if (ev.xmotion.state & Button2Mask)
		    button |= MWBUTTON_M;
		if (ev.xmotion.state & Button3Mask)
		    button |= MWBUTTON_R;
		*bp = button;
		events++;
	    }
	}
	else if (ev.type == ButtonPress) {
	    if (ev.xbutton.window == x11_win) {
	        int button = 0;
		
		/* Get pressed button */
	    	if(ev.xbutton.button == 1)
			button = MWBUTTON_L;
		else if(ev.xbutton.button == 2)
			button = MWBUTTON_M;
		else if(ev.xbutton.button == 3)
			button = MWBUTTON_R;

		/* Get any other buttons that might be already held */
		if (ev.xbutton.state & Button1Mask)
		    button |= MWBUTTON_L;
		if (ev.xbutton.state & Button2Mask)
		    button |= MWBUTTON_M;
		if (ev.xbutton.state & Button3Mask)
		    button |= MWBUTTON_R;
		
/*		DPRINTF("!Pressing button: 0x%x, state: 0x%x, button: 0x%x\n",
			button,ev.xbutton.state, ev.xbutton.button);*/
		*bp = button;
		*dx = ev.xbutton.x;
		*dy = ev.xbutton.y;
		*dz = 0;
		events++;
	    }
	}
	else if (ev.type == ButtonRelease) {
	    if (ev.xbutton.window == x11_win) {
	        int button = 0;
		int released = 0;
	
		/* Get released button */
	    	if(ev.xbutton.button == 1)
			released = MWBUTTON_L;
		else if(ev.xbutton.button == 2)
			released = MWBUTTON_M;
		else if(ev.xbutton.button == 3)
			released = MWBUTTON_R;
		
		/* Get any other buttons that might be already held */
		if (ev.xbutton.state & Button1Mask)
		    button |= MWBUTTON_L;
		if (ev.xbutton.state & Button2Mask)
		    button |= MWBUTTON_M;
		if (ev.xbutton.state & Button3Mask)
		    button |= MWBUTTON_R;
	
		/* We need to remove the released button from the button mask*/
		button &= ~released; 

		*bp = button;
		*dx = ev.xbutton.x;
		*dy = ev.xbutton.y;
		*dz = 0;
		events++;
	    }
	}
	else {
	    x11_handle_event(&ev);
	}
    }
    if (events == 0)
		return MOUSE_NODATA;

    return MOUSE_ABSPOS;		/* absolute position returned*/
}

#pragma once

#include "graphics.h"
#include "panel_out.h"
#include "perf.h"

#include "timer.h"

/* a physical display */
class Panel
{
public:
	Panel(PanelOut &driver);

	Dimension	dimension() { return Dimension(FrameBuffer::columns(), FrameBuffer::rows()); }

	/*
	 * Adjust brightness down.
	 */
	bool		dimmer()
	{
		if ((_max_brightness >> (_dim_level + 1)) > 0) {
			_dim_level++;
			return true;
		}
		return false;
	}

	/*
	 * Adjust brightness up.
	 */
	bool		brighter()
	{
		if (_dim_level > 0) {
			_dim_level--;
			return true;
		}
		return false;
	}

	/*
	 * Get/set current dimming level.
	 */
	unsigned	&dimming() { return _dim_level; }

	/*
	 * XXX come up with some form of raster-dodging or incremental drawing to 
	 *     avoid flicker from overlapping draw operations.
	 */
	void		fill(Colour colour);
	void		clear() { fill(Black); }

	void		draw(Position pos, Colour colour) { _buffer.subCell(pos).set(colour); }

private:
	static const unsigned _depth = PaletteEntry::depth;

	// Brightness is expressed in terms of the on period for the LSB.
	// Assuming a frame time of ~16ms, two rows per line and linear 
	// brightness scaling, determine the LSB period for the given depth.
	static const unsigned _max_brightness = ((16384 / (FrameBuffer::rows() / 2)) >> _depth);

	static void	tick(void *arg);
	void		_tick();
	Timer::Interval	_phase_advance();
	
	FrameBuffer	_buffer;

	Timer		_timer;
	PanelOut	&_driver;

	union {
		unsigned		counter;
		struct {
			unsigned	is_dimming:1;
			unsigned 	row:31-__builtin_clz(FrameBuffer::rows()/2);
			unsigned	slot:32-__builtin_clz(PaletteEntry::depth);
		};
	} 			_phase;
	unsigned	_dim_level;

	PerfInterval	_perf_line_update;		// line update time
	PerfLoad	_load;				// CPU load
};
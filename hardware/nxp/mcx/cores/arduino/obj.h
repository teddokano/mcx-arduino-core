/*
 *  @author Tedd OKANO
 *
 *  Released under the MIT license License
 */

#ifndef R01LIB_BASE_OBJ_H
#define R01LIB_BASE_OBJ_H

/** Common base class for r01lib peripheral driver classes (I2C, SPI,
 *  Serial, DigitalInOut, ...).
 *
 *  Its only job is guaranteeing init_mcu() (chip-level clock/pin bring-up)
 *  runs exactly once, the first time any peripheral object is constructed
 *  -- regardless of which peripheral class gets instantiated first. Every
 *  r01lib peripheral class derives from Obj so this happens automatically
 *  without each of them needing its own explicit "have I initialized the
 *  chip yet?" check.
 *
 *  @class Obj
 */
class Obj
{
public:
	/** @param done currently unused -- init_mcu() is always run on the
	 *              first Obj construction regardless of this value
	 */
	Obj( bool done = false );

	virtual ~Obj();
private:
	static bool	init_done;
};

#endif // R01LIB_BASE_OBJ_H

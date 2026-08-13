/*
 *  @author Tedd OKANO
 *
 *  Released under the MIT license License
 */

#ifndef R01LIB_BASE_OBJ_H
#define R01LIB_BASE_OBJ_H

class Obj
{
public:
	Obj( bool done = false );
	virtual ~Obj();
private:
	static bool	init_done;
};

#endif // R01LIB_BASE_OBJ_H

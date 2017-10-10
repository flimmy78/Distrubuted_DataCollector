#include "../include/DBInterFace.h"
#include "OrcalInterFaceImpl.h"
using namespace WSDBInterFace;

static COrcalObject s_oracalOBJ;
__DB_INTERFACE_API bool WSDBInterFace::CreateDBObject(DBType Type, IDBObject** pDBObject)
{
	if (Type == ORCAL_TYPE)
	{
		*pDBObject = &s_oracalOBJ;
		return true;
	}
	else if (Type == SQLSERVER_TYPE)
	{
#ifdef WIN32

#endif
	}
	return false;
}
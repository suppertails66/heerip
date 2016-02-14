/* Derived class of std::exception to allow construction of exceptions
   with an error message */

#include <exception>
#include <string>

class DefaultException : public std::exception
{
public:
	DefaultException(std::string err)
		: std::exception(), errmess(err) { };
	
	virtual ~DefaultException() throw() { };

	const char* what() throw()
	{
		return errmess.c_str();
	}
private:
	std::string errmess;
};

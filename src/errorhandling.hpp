/*
 * errorhandling.hpp
 *
 *  Created on: Jul 14, 2016
 *      Author: dleo@cwi.nl
 */

#ifndef ERRORHANDLING_HPP_
#define ERRORHANDLING_HPP_

#include <ostream>
#include <sstream>
#include <stdexcept>


namespace gr8 {

/**
 * Base class for custom raised raised errors
 */
class Exception : public std::runtime_error {
private:
    const std::string exceptionClass; // the class of the exception
    const std::string file; // source file where the exception has been raised
    int line; // related line where the exception has been raised
    const std::string function; // function causing the exception

public:
    /**
     * Constructor
     * @param exceptionClass the name of the exception class
     * @param message the error message associated to this exception
     * @param file the source file where the exception has been generated
     * @param line the line where the exception has been generated
     * @param function the function where this exception has been raised
     */
    Exception(const std::string& exceptionClass, const std::string& message, const std::string& file, int line, const std::string& function);

    /**
     * Retrieve the source file where the exception has been raised
     * @return the source file where the exception has been raised
     */
    std::string getFile() const;

    /**
     * The line number where the exception has been raised
     * @return the line number where the exception has been raised
     */
    int getLine() const;

    /**
     * Retrieve the function that fired the exception
     * @return the function name where the exception has been raised
     */
    std::string getFunction() const;

    /**
     * Retrieves the name of the exception class
     * @return the name of the exception class
     */
    std::string getExceptionClass() const;

    /**
     * Utility class to create the exception message
     */
    static thread_local std::stringstream utilitystream;
};

} /* namespace gr8 */

/**
 * Overload the operator to print the descriptive content of an ELF Exception
 */
std::ostream& operator<<(std::ostream& out, gr8::Exception& e);

/**
 * It prepares the arguments `file', `line', `function', and `what' to be passed to an exception ctor
 * @param msg the message stream to concatenate
 */
#define RAISE_EXCEPTION_CREATE_ARGS(msg) const char* file = __FILE__; int line = __LINE__; const char* function = __FUNCTION__; \
        auto& stream = gr8::Exception::utilitystream; \
        stream.str(""); stream.clear(); \
        stream << msg; \
        std::string what = stream.str(); \
        stream.str(""); stream.clear(); /* reset once again */

/**
 * Raises an exception with the given message
 * @param exception the exception to throw
 * @param msg: an implicit ostream, with arguments concatenated with the symbol <<
 */

#define RAISE_EXCEPTION(exc, msg) { RAISE_EXCEPTION_CREATE_ARGS(msg); throw exc ( #exc , what, file, line, function); }

/**
 * Raises the base exception gr8::Exception
 * @param msg: an implicit ostream, with arguments concatenated with the symbol <<
 */
#define RAISE_ERROR(msg) RAISE_EXCEPTION(gr8::Exception, msg);

/**
 * These exception classes are so similar, so define a general macro to create the exception
 */
#define DEFINE_EXCEPTION(exceptionName) class exceptionName: public gr8::Exception { \
	public: exceptionName(const std::string& exceptionClass, const std::string& message, const std::string& file, \
			int line, const std::string& function) : \
		gr8::Exception(exceptionClass, message, file, line, function) { } \
} /* End of DEFINE_EXCEPTION */

/*
 * Raise an exception if the given condition does not hold
 * @param except the exception to raise
 * @param condition the condition to check
 * @param message the message to pass
 */
#define CHECK_EXCEPTION(except, condition, message) if(!(condition)) { RAISE_EXCEPTION(except, message); }

namespace gr8 {

// A wrapper to a MAL error
class MalException : public Exception {
	const char* mal_message;

public:
	MalException(const std::string& exceptionClass, const std::string& message, const std::string& file, \
		int line, const std::string& function, const char* mal_message) : \
				Exception(exceptionClass, message, file, line, function), mal_message(mal_message) { }

	const char* get_mal_error(){ return mal_message; }
};

}

#define MAL_ERROR(mal_error, message) { RAISE_EXCEPTION_CREATE_ARGS(message); throw gr8::MalException("MalException", what, file, line, function, mal_error); }
#define MAL_ASSERT(condition, mal_error) if(!(condition)) { MAL_ERROR(mal_error, ""); }
#define MAL_ASSERT_RC() MAL_ASSERT(rc == MAL_SUCCEED, rc)
#define MAL_ASSERT_MSG(condition, mal_error, message) if(!(condition)) { MAL_ERROR(mal_error, message); }

#endif /* ERRORHANDLING_HPP_ */

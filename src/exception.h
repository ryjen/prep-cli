#ifndef RJ_PREP_EXCEPTION_H
#define RJ_PREP_EXCEPTION_H

namespace rj
{
    namespace prep
    {
        class prep_exception : public std::exception
        {
        public:
            using std::exception::exception;
        };

        class not_a_plugin : public prep_exception
        {
        public:
            using prep_exception::prep_exception;
        };
    }
}


#endif

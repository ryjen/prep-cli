#ifndef ARG3_PREP_EXCEPTION_H
#define ARG3_PREP_EXCEPTION_H

namespace arg3
{
    namespace prep
    {
        class prep_exception : public std::exception
        {
        public:
            using std::exception::exception;
        };

        class revalidate_repository : public prep_exception
        {
        public:
            using prep_exception::prep_exception;
        };

        class not_a_plugin : public prep_exception
        {
        public:
            using prep_exception::prep_exception;
        };
    }
}


#endif

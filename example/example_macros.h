// example_macros.h header file

#ifndef EXAMPLE_MACROS_H
#define EXAMPLE_MACROS_H


#define EXAMPLE_BEGIN \
    void example() { \
      std::cout << "\n\n\nExample begins\n"; \
      std::cout << "===================================================================\n\n"

#define EXAMPLE_END \
    std::cout << "\n\n===================================================================";\
    std::cout << "\nExample ends\n";\
    } // end function example

#define EXAMPLE_RUN \
    example()

#define EXAMPLE(example_message) \
    std::cout << "\n\n" << example_message << "\n"; \
    std::cout << "===================================================================\n";\



#endif // EXAMPLE_MACROS_H

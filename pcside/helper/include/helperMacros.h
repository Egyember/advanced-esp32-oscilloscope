#pragma once
#include <chrono>
#include <iostream>

#define STARTTIMEER(x) auto start##x = std::chrono::high_resolution_clock::now()
#define STOPTIMEER(x) (std::chrono::duration<float>(std::chrono::high_resolution_clock::now() - start##x).count())
#define STOPANDPRINTTIMER(x)  std::cout << "##x took " << STOPTIMEER(x) << "s\n"

#include <iostream>
#include <chrono>
#include <vector>
#include <string>

// Mock FName
struct FName {
    int Index;
    int Number;

    FName(const char* Name) : Index(0), Number(0) {
        // Simulate some lookup work
        std::string s(Name);
        Index = s.length();
    }

    FName(const FName& Other) : Index(Other.Index), Number(Other.Number) {}
};

// Mock FMaterialParameterInfo
struct FMaterialParameterInfo {
    FName Name;
    int Association;
    int Index;

    FMaterialParameterInfo(const FName& InName)
        : Name(InName)
        , Association(0)
        , Index(-1)
    {}
};

// Volatile to prevent optimization
volatile int Sink = 0;

void TestBaseline(const FName& ParamName, int Iterations) {
    auto Start = std::chrono::high_resolution_clock::now();

    for(int i=0; i<Iterations; ++i) {
        FMaterialParameterInfo Info(ParamName);
        Sink += Info.Index; // Prevent optimization
    }

    auto End = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> Elapsed = End - Start;
    std::cout << "Baseline: " << Elapsed.count() << " ms" << std::endl;
}

void TestOptimized(const FName& ParamName, int Iterations) {
    auto Start = std::chrono::high_resolution_clock::now();

    for(int i=0; i<Iterations; ++i) {
        static const FMaterialParameterInfo Info(ParamName);
        Sink += Info.Index; // Prevent optimization
    }

    auto End = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> Elapsed = End - Start;
    std::cout << "Optimized: " << Elapsed.count() << " ms" << std::endl;
}

int main() {
    const int Iterations = 100000000;
    FName ParamName("Progress");

    std::cout << "Running benchmark with " << Iterations << " iterations..." << std::endl;

    TestBaseline(ParamName, Iterations);
    TestOptimized(ParamName, Iterations);

    return 0;
}

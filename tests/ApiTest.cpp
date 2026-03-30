#include <cstdlib>
#include <iostream>
#include <string>

int main(int argc, char** argv)
{
    std::cout << "======================================" << std::endl;
    std::cout << "   HTTP API Test Runner for Ciallo" << std::endl;
    std::cout << "======================================" << std::endl;
    std::cout << std::endl;

    std::cout << "Note: Run './test_api.sh' for curl-based HTTP tests" << std::endl;
    std::cout << std::endl;

    std::cout << "This test requires:" << std::endl;
    std::cout << "  1. Server running on https://localhost:1145" << std::endl;
    std::cout << "  2. curl installed" << std::endl;
    std::cout << "  3. Valid SSL certificates in ca/" << std::endl;
    std::cout << std::endl;

    std::cout << "Usage:" << std::endl;
    std::cout << "  1. Start the server: ./build/oat_quick_start" << std::endl;
    std::cout << "  2. Run tests: ./tests/test_api.sh" << std::endl;
    std::cout << "  Or combined: ./build/oat_quick_start & ./tests/test_api.sh" << std::endl;
    std::cout << std::endl;

    std::cout << "Test Endpoints:" << std::endl;
    std::cout << "  - GET  /health              - Health check" << std::endl;
    std::cout << "  - POST /api/auth/login     - Login" << std::endl;
    std::cout << "  - POST /api/auth/refresh   - Refresh token" << std::endl;
    std::cout << "  - POST /api/auth/logout    - Logout" << std::endl;
    std::cout << "  - GET  /api/users          - List users (admin)" << std::endl;
    std::cout << "  - GET  /api/photos         - List photos" << std::endl;
    std::cout << "  - POST /api/photos/upload  - Upload photo" << std::endl;
    std::cout << "  - POST /api/photos/init    - Init chunk upload" << std::endl;
    std::cout << "  - POST /api/photos/chunk   - Upload chunk" << std::endl;
    std::cout << "  - POST /api/photos/complete - Complete upload" << std::endl;
    std::cout << "  - GET  /api/photos/{id}   - Get photo" << std::endl;
    std::cout << "  - DELETE /api/photos/{id}  - Delete photo" << std::endl;
    std::cout << std::endl;

    return 0;
}

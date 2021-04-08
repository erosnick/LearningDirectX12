// int main() {
    // std::cout.setf(std::ios_base::boolalpha);

    // // 检查是否支持SSE2指令集
    // if (!XMVerifyCPUSupport()) {
    //     std::cout << "DirectXMath not supported." << std::endl;
    //     return 0;
    // }

    // XMVECTOR p = XMVectorZero();
    // XMVECTOR q = XMVectorSplatOne();
    // XMVECTOR u = XMVectorSet(1.0f, 2.0f, 3.0f, 0.0f);
    // XMVECTOR v = XMVectorReplicate(-2.0f);
    // XMVECTOR w = XMVectorSplatZ(u);

    // std::cout << "p = " << p << std::endl;
    // std::cout << "q = " << q << std::endl;
    // std::cout << "u = " << u << std::endl;
    // std::cout << "v = " << v << std::endl;
    // std::cout << "w = " << w << std::endl;

    // XMMATRIX A(1.0f, 0.0f, 0.0f, 0.0f,
    //            0.0f, 2.0f, 0.0f, 0.0f,
    //            0.0f, 0.0f, 4.0f, 0.0f,
    //            1.0f, 2.0f, 3.0f, 1.0f);

    // XMMATRIX B = XMMatrixIdentity();

    // XMMATRIX C = A * B;

    // XMMATRIX D = XMMatrixTranspose(A);

    // XMVECTOR det = XMMatrixDeterminant(A);

    // XMMATRIX E = XMMatrixInverse(&det, A);

    // XMMATRIX F = A * E;

    // std::cout << "A = " << std::endl << A << std::endl;
    // std::cout << "B = " << std::endl << B << std::endl;
    // std::cout << "C = A * B = " << std::endl << C << std::endl;
    // std::cout << "D = transpose(A) = " << std::endl << D << std::endl;
    // std::cout << "E = inverse(A) = " << std::endl << E << std::endl;
    // std::cout << "F = A * E = " << std::endl << F << std::endl;

//     createDevice();

//     createCommandObjects();

//     test t;

//     Vertex v0;
//     Vertex v1;

//     t.foo(v0, v1);

//     return 0;
// }
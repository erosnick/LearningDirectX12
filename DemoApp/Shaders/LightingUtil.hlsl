struct Light {
    float3 Strength;
    float FalloffStart;
    float3 Direction;
    float FalloffEnd;
    float3 Position;
    float SpotPower;
};

struct Material {
    float4 DiffuseAlbedo;
    float3 FresnealR0;

    // 光泽度与粗糙度是一对性质相反的属性：
    // Shininess = 1 - roughness
    float Shininess;
};

float CalculateAttenuation(float distance, float falloffStart, float falloffEnd) {
    // 线性衰减
    return staturate((falloffEnd - distance) / (falloffEnd - falloffStart));
}

// 石里克提出的一种逼近菲涅尔反射率的近似方法
// R0 = ((n - 1)/(n + 1))^2，式中的n为折射率
// R0 + (1 - R0)(1 - cosθ)^5
// θ是法线(h = L + v, L和v之间的半角向量)和光照向量之间的夹角
float3 SchlickFresnel(float3 R0, float3 normal, float3 lightVector) {
    float cosIncidentAngle = saturate(dot(normal, lightVector));

    float f0 = 1.0f - cosIncidentAngle;
    float3 reflectPercent = R0 + (1.0f - R0) * (f0 * f0 * f0 * f0 * f0);

    return reflectPercent;
}

float BlinnPhong(float3 lightStrength, float3 lightVector, float3 normal, float3 toEye, Material material) {
    // m由光泽度推导而来，而光泽度则根据粗糙度求得
    const float m = material.Shininess * 256.0f;
    float3 halfVector = normalize(toEye + lightVector);

    float roughnessFactor = (m + 8.0f) * pow(max(dot(halfVector, normal), 0.0f), m) / 8.0f;

    float3 fresnelFactor = SchlickFresnel(material.FresnealR0, halfVector, lightVector);

    float3 specularAlbedo = roughnessFactor * fresnealFactor;

    // 尽管我们进行的是LDR(low dynamic range, 低动态范围)渲染，但spec(镜面反射)公式得到
    // 的结果仍然会超过范围[0, 1]，因此现将其按比例缩小一些
    specularAlbedo = specularAlbedo / (specularAlbedo + 1.0f);

    return (material.DiffuseAlbedo.rgb + specularAlbedo) * lightStrength;
}
struct Light {
    float3 Strength;
    float FalloffStart;
    float3 Direction;
    float FalloffEnd;
    float3 Position;
    float SpotPower;
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
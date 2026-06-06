#pragma once
#include <openxr/openxr.h>
#include <cmath>

struct Matrix4x4 {
    float m[16];
};

inline void Matrix4x4_Multiply(Matrix4x4* result, const Matrix4x4* a, const Matrix4x4* b) {
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            float sum = 0.0f;
            for (int k = 0; k < 4; ++k) {
                sum += a->m[k * 4 + i] * b->m[j * 4 + k];
            }
            result->m[j * 4 + i] = sum;
        }
    }
}

inline void CreateProjectionMatrix(Matrix4x4* result, const XrFovf fov, const float nearZ, const float farZ) {
    const float tanAngleLeft = tanf(fov.angleLeft);
    const float tanAngleRight = tanf(fov.angleRight);
    const float tanAngleDown = tanf(fov.angleDown);
    const float tanAngleUp = tanf(fov.angleUp);

    const float tanAngleWidth = tanAngleRight - tanAngleLeft;
    const float tanAngleHeight = tanAngleUp - tanAngleDown;

    for (int i = 0; i < 16; ++i) result->m[i] = 0.0f;

    result->m[0] = 2.0f / tanAngleWidth;
    result->m[4] = 0.0f;
    result->m[8] = (tanAngleRight + tanAngleLeft) / tanAngleWidth;
    result->m[12] = 0.0f;

    result->m[1] = 0.0f;
    result->m[5] = -2.0f / tanAngleHeight;
    result->m[9] = -(tanAngleUp + tanAngleDown) / tanAngleHeight;
    result->m[13] = 0.0f;

    result->m[2] = 0.0f;
    result->m[6] = 0.0f;
    result->m[10] = -(farZ + nearZ) / (farZ - nearZ);
    result->m[14] = -(2.0f * farZ * nearZ) / (farZ - nearZ);

    result->m[3] = 0.0f;
    result->m[7] = 0.0f;
    result->m[11] = -1.0f;
    result->m[15] = 0.0f;
}

inline void CreatePoseMatrix(Matrix4x4* result, const XrPosef pose) {
    const float x = pose.orientation.x;
    const float y = pose.orientation.y;
    const float z = pose.orientation.z;
    const float w = pose.orientation.w;

    const float x2 = x + x;
    const float y2 = y + y;
    const float z2 = z + z;
    const float xx = x * x2;
    const float xy = x * y2;
    const float xz = x * z2;
    const float yy = y * y2;
    const float yz = y * z2;
    const float zz = z * z2;
    const float wx = w * x2;
    const float wy = w * y2;
    const float wz = w * z2;

    result->m[0] = 1.0f - (yy + zz);
    result->m[1] = xy + wz;
    result->m[2] = xz - wy;
    result->m[3] = 0.0f;

    result->m[4] = xy - wz;
    result->m[5] = 1.0f - (xx + zz);
    result->m[6] = yz + wx;
    result->m[7] = 0.0f;

    result->m[8] = xz + wy;
    result->m[9] = yz - wx;
    result->m[10] = 1.0f - (xx + yy);
    result->m[11] = 0.0f;

    result->m[12] = pose.position.x;
    result->m[13] = pose.position.y;
    result->m[14] = pose.position.z;
    result->m[15] = 1.0f;
}

inline void CreateViewMatrix(Matrix4x4* result, const XrPosef pose) {
    Matrix4x4 poseMat;
    CreatePoseMatrix(&poseMat, pose);

    float m[16];
    for (int i=0; i<16; i++) m[i] = poseMat.m[i];
    
    result->m[0] = m[0]; result->m[1] = m[4]; result->m[2] = m[8]; result->m[3] = 0.0f;
    result->m[4] = m[1]; result->m[5] = m[5]; result->m[6] = m[9]; result->m[7] = 0.0f;
    result->m[8] = m[2]; result->m[9] = m[6]; result->m[10] = m[10]; result->m[11] = 0.0f;
    
    result->m[12] = -(m[0] * m[12] + m[1] * m[13] + m[2] * m[14]);
    result->m[13] = -(m[4] * m[12] + m[5] * m[13] + m[6] * m[14]);
    result->m[14] = -(m[8] * m[12] + m[9] * m[13] + m[10] * m[14]);
    result->m[15] = 1.0f;
}

inline void CreateModelMatrix(Matrix4x4* result, const XrPosef pose, const float scale[3]) {
    CreatePoseMatrix(result, pose);
    for (int i = 0; i < 4; ++i) {
        result->m[0 * 4 + i] *= scale[0];
        result->m[1 * 4 + i] *= scale[1];
        result->m[2 * 4 + i] *= scale[2];
    }
}

inline void Matrix4x4_RotateY(Matrix4x4* result, const Matrix4x4* m, float angleDegrees) {
    float radians = angleDegrees * 3.14159265358979323846f / 180.0f;
    float c = cosf(radians);
    float s = sinf(radians);
    
    Matrix4x4 rot = {
        c, 0, s, 0,
        0, 1, 0, 0,
       -s, 0, c, 0,
        0, 0, 0, 1
    };
    Matrix4x4_Multiply(result, m, &rot);
}

inline void Matrix4x4_RotateX(Matrix4x4* result, const Matrix4x4* m, float angleDegrees) {
    float radians = angleDegrees * 3.14159265358979323846f / 180.0f;
    float c = cosf(radians);
    float s = sinf(radians);
    
    Matrix4x4 rot = {
        1, 0,  0, 0,
        0, c,  s, 0,
        0,-s,  c, 0,
        0, 0,  0, 1
    };
    Matrix4x4_Multiply(result, m, &rot);
}

inline void CreateBoneMatrix(Matrix4x4* result, const XrVector3f& pA, const XrVector3f& pB, float thickness) {
    float dx = pB.x - pA.x;
    float dy = pB.y - pA.y;
    float dz = pB.z - pA.z;
    float len = sqrtf(dx*dx + dy*dy + dz*dz);
    if (len < 0.0001f) {
        for (int i=0; i<16; i++) result->m[i] = 0.0f;
        result->m[0] = result->m[5] = result->m[10] = result->m[15] = 1.0f;
        return;
    }

    float ux = dx / len;
    float uy = dy / len;
    float uz = dz / len;

    float vx, vy, vz;
    if (fabsf(ux) < 0.9f) {
        vx = 1.0f; vy = 0.0f; vz = 0.0f;
    } else {
        vx = 0.0f; vy = 1.0f; vz = 0.0f;
    }

    float rx = uy * vz - uz * vy;
    float ry = uz * vx - ux * vz;
    float rz = ux * vy - uy * vx;
    float rlen = sqrtf(rx*rx + ry*ry + rz*rz);
    rx /= rlen; ry /= rlen; rz /= rlen;

    vx = ry * uz - rz * uy;
    vy = rz * ux - rx * uz;
    vz = rx * uy - ry * ux;

    float cx = (pA.x + pB.x) * 0.5f;
    float cy = (pA.y + pB.y) * 0.5f;
    float cz = (pA.z + pB.z) * 0.5f;

    result->m[0] = rx * thickness;
    result->m[1] = ry * thickness;
    result->m[2] = rz * thickness;
    result->m[3] = 0.0f;

    result->m[4] = vx * thickness;
    result->m[5] = vy * thickness;
    result->m[6] = vz * thickness;
    result->m[7] = 0.0f;

    result->m[8] = ux * len;
    result->m[9] = uy * len;
    result->m[10] = uz * len;
    result->m[11] = 0.0f;

    result->m[12] = cx;
    result->m[13] = cy;
    result->m[14] = cz;
    result->m[15] = 1.0f;
}

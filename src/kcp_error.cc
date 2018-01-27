#include "kcp_error.h"

namespace kcp {
std::string ErrorCategory::message(int err) const {
    switch (err) {
    case ErrNum::kInvalidArgment:
        return "invalid argment";
    case ErrNum::kUDPSendFailed:
        return "udp send failed";
    case ErrNum::kBadIP4Address:
        return "bad udp address";
    case ErrNum::kKCPInputFailed:
        return "kcp input failed";
    case ErrNum::kBadUDPMsg:
        return "bad udp msg";
    case ErrNum::kBadKCPConv:
        return "bad kcp conv";
    default:
        break;
    }

    return "unknown error";
}

const ErrorCategory& GetErrorCategory() {
    static ErrorCategory _category;
    return _category;
}

std::error_code MakeErrorCode(ErrNum err) {
    return std::error_code(static_cast<int>(err), GetErrorCategory());
}
}


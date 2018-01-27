#ifndef _KCP_ERROR_H_INCLUDED
#define _KCP_ERROR_H_INCLUDED

#include <system_error>

namespace kcp {
enum class ErrNum : uint8_t {
    kInvalidArgment = 1,
    kUDPSendFailed,
    kBadIP4Address,
    kKCPInputFailed,
    kBadUDPMsg,
    kBadKCPConv
};

class ErrorCategory : public std::error_category {
public:
    const char *name() const noexcept override { return "kcp"; }
    std::string message(int err) const override;
};

const ErrorCategory& GetErrorCategory();
std::error_code MakeErrorCode(ErrNum);
}

#endif // !_KCP_ERROR_H_INCLUDED

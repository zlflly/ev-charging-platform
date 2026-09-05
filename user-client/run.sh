#!/usr/bin/env bash
# 用户客户端一键运行脚本：按需配置 CMake、增量构建，然后带上联调需要的环境变量启动。
set -euo pipefail

readonly CLIENT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
readonly BUILD_DIR="${CLIENT_ROOT}/build"
readonly BINARY_PATH="${BUILD_DIR}/user-client"
readonly VALID_PREVIEW_PAGES=(home station charger navigation charging profile)

previewPage=""
shouldBuild=1
shouldClean=0
enableMapDebug=0
logPath=""

printUsage()
{
    cat <<'USAGE'
用法：./run.sh [选项] [-- 传给可执行文件的参数]

选项：
  -p, --preview PAGE    以界面预览模式直达某个页面（自动注入预览登录态）
                        可选值：home station charger navigation charging profile
  -n, --no-build        跳过构建，直接运行已有产物
  -c, --clean           删除 build 目录后重新配置并全量构建
  -m, --map-debug       打开 QtWebEngine / 地图诊断日志
  -l, --log FILE        同时把运行日志写入文件
  -h, --help            显示本帮助

示例：
  ./run.sh                       # 构建并正常启动（需要先启动服务端）
  ./run.sh -p home               # 预览首页，不依赖服务端登录
  ./run.sh -m -l /tmp/uc.log     # 排查地图问题时保留完整日志
USAGE
}

isValidPreviewPage()
{
    local candidate="$1"
    local page
    for page in "${VALID_PREVIEW_PAGES[@]}"; do
        [[ "${page}" == "${candidate}" ]] && return 0
    done
    return 1
}

passthroughArguments=()
while [[ $# -gt 0 ]]; do
    case "$1" in
        -p|--preview)
            [[ $# -ge 2 ]] || { echo "错误：${1} 需要一个页面名。" >&2; exit 2; }
            previewPage="$2"
            isValidPreviewPage "${previewPage}" \
                || { echo "错误：未知页面 '${previewPage}'，可选：${VALID_PREVIEW_PAGES[*]}" >&2; exit 2; }
            shift 2
            ;;
        -n|--no-build) shouldBuild=0; shift ;;
        -c|--clean) shouldClean=1; shift ;;
        -m|--map-debug) enableMapDebug=1; shift ;;
        -l|--log)
            [[ $# -ge 2 ]] || { echo "错误：${1} 需要一个文件路径。" >&2; exit 2; }
            logPath="$2"
            shift 2
            ;;
        -h|--help) printUsage; exit 0 ;;
        --) shift; passthroughArguments+=("$@"); break ;;
        *) echo "错误：无法识别的参数 '${1}'，用 --help 查看用法。" >&2; exit 2 ;;
    esac
done

cd "${CLIENT_ROOT}"

if [[ ${shouldClean} -eq 1 ]]; then
    echo "==> 清理 ${BUILD_DIR}"
    rm -rf "${BUILD_DIR}"
fi

if [[ ${shouldBuild} -eq 1 ]]; then
    if [[ ! -f "${BUILD_DIR}/CMakeCache.txt" ]]; then
        echo "==> 配置 CMake"
        cmake -S . -B build
    fi
    echo "==> 构建"
    cmake --build build -j"$(nproc)"
fi

if [[ ! -x "${BINARY_PATH}" ]]; then
    echo "错误：找不到可执行文件 ${BINARY_PATH}，请去掉 --no-build 后重试。" >&2
    exit 1
fi

if [[ ! -f "${CLIENT_ROOT}/local.env" ]]; then
    echo "提示：未找到 local.env，地图与定位功能将不可用；" >&2
    echo "      可复制 local.env.example 并填入自己的高德 key。" >&2
fi

if [[ -n "${previewPage}" ]]; then
    # 预览模式注入本地登录态与定位，用来单独检查页面视觉，不代表真实业务状态。
    export EV_PREVIEW_AUTH=1
    export EV_PREVIEW_PAGE="${previewPage}"
    echo "==> 预览模式：${previewPage}"
fi

if [[ ${enableMapDebug} -eq 1 ]]; then
    export QT_LOGGING_RULES="qt.webenginecontext.debug=true"
    echo "==> 已打开地图诊断日志"
fi

echo "==> 启动 user-client"
if [[ -n "${logPath}" ]]; then
    exec "${BINARY_PATH}" "${passthroughArguments[@]+"${passthroughArguments[@]}"}" 2>&1 | tee "${logPath}"
fi
exec "${BINARY_PATH}" "${passthroughArguments[@]+"${passthroughArguments[@]}"}"

project_root = "../../../.."
include(project_root.."/tools/build")

test_suite("xenia-apu-tests", project_root, ".", {
  links = {
    "libavcodec",
    "libavutil",
    "xenia-apu",
    "xenia-base",
  },
})

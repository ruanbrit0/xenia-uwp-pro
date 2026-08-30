project_root = "../../../.."
include(project_root.."/tools/build")

test_suite("xenia-kernel-tests", project_root, ".", {
  links = {
    "capstone",
    "fmt",
    "imgui",
    "xenia-apu",
    "xenia-base",
    "xenia-core",
    "xenia-cpu",
    "xenia-gpu",
    "xenia-hid",
    "xenia-kernel",
    "xenia-patcher",
    "xenia-ui",
    "xenia-vfs",
  },
  filtered_links = {
    {
      filter = "architecture:x86_64",
      links = {
        "xenia-cpu-backend-x64",
      },
    }
  },
})

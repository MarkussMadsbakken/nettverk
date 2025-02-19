defmodule Ov5Web.IndexLive do
  use Ov5Web, :live_view

  def mount(_params, _session, socket) do
    {:ok,
     socket
     |> assign(code: "")
     |> assign(output: "")
     |> assign(error: "")
     |> assign(compiling: false)
     |> assign(running: false)
     |> assign(container_id: "")}
  end

  def handle_event("compile", %{"code" => code}, socket) do
    pid = self()
    Task.start(fn -> compile_and_run(pid, code) end)

    {:noreply,
     socket
     |> assign(output: "")
     |> assign(error: "")
     |> assign(compiling: true)
     |> assign(running: false)
     |> assign(code: code)
     |> assign(contaner_id: "")}
  end

  def handle_event("stop_program", _params, socket) do
    System.cmd("docker", ["stop", "ov5-compiler-#{socket.assigns.container_id}"],
      into: IO.stream()
    )

    {:noreply, socket |> assign(running: false) |> assign(container_id: "")}
  end

  def handle_info([build_error: %{exit_code: exit_code, output: output}], socket) do
    error =
      String.split(output, "RUN gcc -o app")
      |> Enum.at(1)
      |> String.split("error: ")
      |> Enum.at(1)
      |> String.split("\n")
      |> Enum.at(0)

    error = "Build exited with exit code #{exit_code}:\n #{error}"

    {:noreply,
     socket
     |> assign(output: "")
     |> assign(error: error)
     |> assign(compiling: false)
     |> assign(running: false)}
  end

  def handle_info([program_output: %{exit_code: exit_code, output: output}], socket) do
    if(exit_code != 0) do
      error = "Program exited with exit code #{exit_code}:\n #{output}"
      {:noreply, socket |> assign(output: "") |> assign(error: error) |> assign(compiling: false)}
    else
      {:noreply,
       socket
       |> assign(output: output)
       |> assign(error: "")
       |> assign(compiling: false)
       |> assign(running: false)}
    end
  end

  def handle_info([program_running: container_id], socket) do
    {:noreply,
     socket
     |> assign(output: "")
     |> assign(error: "")
     |> assign(compiling: false)
     |> assign(running: true)
     |> assign(container_id: container_id)}
  end

  def compile_and_run(pid, code) do
    id = UUID.uuid4()
    File.write!("lib/ov5/compiler/tmp/#{id}.c", code)

    {build_output, exit_code} =
      System.cmd(
        "docker",
        [
          "build",
          "-t",
          "ov5-compiler",
          "lib/ov5/compiler",
          "--build-arg",
          "PROGRAM_ID=#{id}"
        ],
        stderr_to_stdout: true
      )

    if(exit_code != 0) do
      send(pid,
        build_error: %{exit_code: exit_code, output: build_output}
      )
    else
      send(pid, program_running: id)

      {program_output, program_exit_code} =
        System.cmd("docker", ["run", "--rm", "--name", "ov5-compiler-#{id}", "ov5-compiler"],
          stderr_to_stdout: true
        )

      send(pid, program_output: %{exit_code: program_exit_code, output: program_output})
    end

    File.rm!("lib/ov5/compiler/tmp/#{id}.c")
  end
end

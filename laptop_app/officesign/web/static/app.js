const messageTypeSelect = document.getElementById("message-type");
const forms = document.querySelectorAll(".message-form");
const resultEl = document.getElementById("result");

function showFormFor(value) {
  forms.forEach((form) => {
    form.hidden = form.id !== `${value}-form`;
  });
}

messageTypeSelect.addEventListener("change", () => showFormFor(messageTypeSelect.value));
showFormFor(messageTypeSelect.value);

async function postJSON(path, body) {
  const resp = await fetch(path, {
    method: "POST",
    headers: { "Content-Type": "application/json" },
    body: body === undefined ? undefined : JSON.stringify(body),
  });
  const data = await resp.json().catch(() => ({}));
  if (!resp.ok) {
    throw new Error(data.detail || `Request to ${path} failed (${resp.status})`);
  }
  return data;
}

async function handleSubmit(event) {
  event.preventDefault();
  const form = event.target;
  const formData = new FormData(form);

  try {
    switch (form.id) {
      case "clock-mode-form":
        await postJSON("/api/messages/clock-mode", {
          show_weather: formData.get("show_weather") === "on",
        });
        break;
      case "meeting-mode-form":
        await postJSON("/api/messages/meeting-mode", {
          duration_minutes: Number(formData.get("duration_minutes")),
          participant_name: formData.get("participant_name"),
          tz_offset_minutes: Number(formData.get("tz_offset_minutes")),
          style: Number(formData.get("style")),
        });
        break;
      case "weather-form":
        await postJSON("/api/messages/weather", {
          condition: Number(formData.get("condition")),
          current_f: Number(formData.get("current_f")),
          high_f: Number(formData.get("high_f")),
          low_f: Number(formData.get("low_f")),
        });
        break;
      case "vibe-mode-form":
        await postJSON("/api/messages/vibe-mode");
        break;
      case "go-dark-form":
        await postJSON("/api/messages/go-dark");
        break;
      case "set-time-form":
        await postJSON("/api/messages/set-time");
        break;
    }
    resultEl.textContent = "Sent.";
  } catch (err) {
    resultEl.textContent = `Error: ${err.message}`;
  }
  refreshStatus();
}

forms.forEach((form) => form.addEventListener("submit", handleSubmit));

document.getElementById("use-next-meeting").addEventListener("click", async () => {
  try {
    const meeting = await (await fetch("/api/calendar/next-meeting")).json();
    if (!meeting.found) {
      resultEl.textContent = "No upcoming meeting found.";
      return;
    }
    const form = document.getElementById("meeting-mode-form");
    form.elements.namedItem("duration_minutes").value = meeting.duration_minutes;
    form.elements.namedItem("participant_name").value = meeting.participant_name;
    resultEl.textContent = `Loaded "${meeting.title}" from your calendar.`;
  } catch (err) {
    resultEl.textContent = `Error: ${err.message}`;
  }
});

async function refreshStatus() {
  try {
    const status = await (await fetch("/api/status")).json();
    document.getElementById("connection-indicator").textContent = status.connected
      ? "Connected to OfficeSign"
      : "Not connected";
    document.getElementById("weather-indicator").textContent = status.last_weather
      ? `Last weather: ${status.last_weather.current_f}F`
      : "";
  } catch (err) {
    document.getElementById("connection-indicator").textContent = "Status unavailable";
  }
}

refreshStatus();
setInterval(refreshStatus, 30000);

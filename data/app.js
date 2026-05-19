document.addEventListener("DOMContentLoaded", () => {
  const loginForm = document.getElementById("loginForm");
  const usernameInput = document.getElementById("username");
  const passwordInput = document.getElementById("password");

  loginForm?.addEventListener("submit", (e) => {
    e.preventDefault();
    const username = usernameInput.value.trim().toLowerCase();
    const password = passwordInput.value.trim();
    if (username === "ravabot" && password === "1122334455") {
      window.location.href = "/dashboard.html";
    } else {
      usernameInput.value = "";
      passwordInput.value = "";
      alert("Invalid username or password. Try again.");
    }
  });

  // ===== Theme Toggle =====
  const themeBtn = document.getElementById("themeBtn");
  themeBtn?.addEventListener("click", () => {
    themeBtn.classList.toggle("fa-sun-o");
    themeBtn.classList.toggle("fa-moon-o");
    themeBtn.classList.toggle("light-mode");
    themeBtn.classList.toggle("dark-mode");
    document.body.classList.toggle("dark-theme");
  });

  // ===== Modal =====
  const modal = document.getElementById("myModal");
  const openBtn = document.getElementById("openModalBtn");
  const closeBtn = document.getElementById("closeModalBtn");
  const configForm = document.getElementById("configForm");

  openBtn?.addEventListener("click", () => {
    modal.style.display = "flex";

    getSettingsData();
  });
  closeBtn?.addEventListener("click", () => (modal.style.display = "none"));
  window.addEventListener("click", (e) => {
    if (e.target === modal) modal.style.display = "none";
  });

  configForm?.addEventListener("submit", async (e) => {
    e.preventDefault();
    const tempThreshold = document.getElementById("tempThreshold")?.value ?? "";
    const minBattery = document.getElementById("minBattery")?.value ?? "";
    const batRange1 = document.getElementById("range1")?.value ?? "";
    const batRange2 = document.getElementById("range2")?.value ?? "";
    const batRange3 = document.getElementById("range3")?.value ?? "";
    const batRange4 = document.getElementById("range4")?.value ?? "";

    const body = new URLSearchParams();
    body.append("minBattery", minBattery);
    body.append("tempThreshold", tempThreshold);
    body.append("batRange1", batRange1);
    body.append("batRange2", batRange2);
    body.append("batRange3", batRange3);
    body.append("batRange4", batRange4);

    try {
      const r = await fetch("/setData", {
        method: "POST",
        headers: { "Content-Type": "application/x-www-form-urlencoded" },
        body: body.toString(),
      });
      if (!r.ok) throw new Error(`HTTP ${r.status}`);
      alert("Configuration saved");
    } catch (err) {
      console.error("save config failed:", err);
      alert("Failed to save config");
    }
  });

  // ===== Inverter / PHCN Toggle =====
  const toggleBtn = document.getElementById("toggleBtn");

  function setSwitchIcon(isGrid) {
    toggleBtn?.classList.toggle("fa-toggle-on", isGrid);
    toggleBtn?.classList.toggle("fa-toggle-off", !isGrid);
    toggleBtn?.classList.toggle("toggle-on", isGrid);
    toggleBtn?.classList.toggle("toggle-off", !isGrid);
  }

  async function postSwitch(isGrid) {
    const body = new URLSearchParams();
    body.append("switch2grid", isGrid ? 1 : 0);
    try {
      const r = await fetch("/setData", {
        method: "POST",
        headers: { "Content-Type": "application/x-www-form-urlencoded" },
        body: body.toString(),
      });
      if (!r.ok) throw new Error(`HTTP ${r.status}`);
    } catch (e) {
      console.error("switch2grid update failed:", e);
    }
  }

  toggleBtn?.addEventListener("click", async () => {
    const nowGrid = toggleBtn.classList.contains("fa-toggle-off");
    setSwitchIcon(nowGrid);
    await postSwitch(nowGrid);
  });

  // ===== Data fetch/render =====
  async function getData() {
    try {
      const res = await fetch("/getData", { cache: "no-store" });
      if (!res.ok) throw new Error(`HTTP ${res.status}`);
      const data = await res.json();
      console.log("Data:", data);
      renderData(data);
    } catch (err) {
      console.error("getData() failed:", err);
    }
  }
  // ===== Data fetch/render =====
  async function getSettingsData() {
    try {
      const res = await fetch("/getSettingsData", { cache: "no-store" });
      if (!res.ok) throw new Error(`HTTP ${res.status}`);
      const data = await res.json();
      console.log("Data Setting:", data);
      prefillConfig(data);
    } catch (err) {
      console.error("getSettingsData() failed:", err);
    }
  }
  function prefillConfig(d) {
    const minTempEl = document.getElementById("tempThreshold");
    if (minTempEl) {
      minTempEl.value = d.tempThreshold ?? "";
    }
    const r1 = document.getElementById("range1");
    if (r1) r1.value = d.batRange1 ?? "";
    const r2 = document.getElementById("range2");
    if (r2) r2.value = d.batRange2 ?? "";
    const r3 = document.getElementById("range3");
    if (r3) r3.value = d.batRange3 ?? "";
    const r4 = document.getElementById("range4");
    if (r4) r4.value = d.batRange4 ?? "";
    const mb = document.getElementById("minBattery");
    if (mb && typeof d.minBattery !== "undefined") mb.value = d.minBattery;
  }
  function renderData(d) {
    setText("batteryPercent", `${d.batteryPercent}%`);
    setText("current", `${d.current}A`);
    setText("voltage", `${d.voltage}V`);
    setText("energy", `${d.energy}kWh`);
    setText("power", `${d.power}W`);
    setText("frequency", `${d.frequency}Hz`);
    setText("powerFactor", `${d.powerFactor}`);
    setText("temperature", `${d.temperature}°C`);
    setStatus("fanStatus", !!d.fanStatus);
    setStatus("socketsStatus", !!d.socketsStatus);
    setStatus("lightStatus", !!d.lightStatus);
    setStatus("displayStatus", !!d.displayStatus);
    setSwitchIcon(!!d.switch2grid);
  }

  function setText(id, val) {
    const el = document.getElementById(id);
    if (!el) {
      console.warn(`[setText] Missing element #${id}`);
      return;
    }
    el.textContent = val ?? "--";
  }

  function setStatus(id, isOn) {
    const el = document.getElementById(id);
    if (!el) {
      console.warn(`[setStatus] Missing element #${id}`);
      return;
    }
    el.textContent = isOn ? "Status: ON" : "Status: OFF";
  }

  getData();
  setInterval(getData, 1000);
});

let trace = null;
let currentIndex = 0;
let playTimer = null;

const traceFileInput = document.getElementById("traceFile");
const prevBtn = document.getElementById("prevBtn");
const nextBtn = document.getElementById("nextBtn");
const playBtn = document.getElementById("playBtn");
const pauseBtn = document.getElementById("pauseBtn");
const cycleSlider = document.getElementById("cycleSlider");

const cycleText = document.getElementById("cycleText");
const pcText = document.getElementById("pcText");

const issuedText = document.getElementById("issuedText");
const cdbText = document.getElementById("cdbText");
const commitText = document.getElementById("commitText");

const robHeadText = document.getElementById("robHeadText");
const robTailText = document.getElementById("robTailText");
const robCountText = document.getElementById("robCountText");

const eventsList = document.getElementById("eventsList");
const robEntries = document.getElementById("robEntries");
const activeInstructions = document.getElementById("activeInstructions");
const lsqEntries = document.getElementById("lsqEntries");

traceFileInput.addEventListener("change", handleTraceFile);
prevBtn.addEventListener("click", previousCycle);
nextBtn.addEventListener("click", nextCycle);
playBtn.addEventListener("click", play);
pauseBtn.addEventListener("click", pause);

cycleSlider.addEventListener("input", () => {
  if (!trace) return;
  currentIndex = Number(cycleSlider.value);
  render();
});

function handleTraceFile(event) {
  const file = event.target.files[0];
  if (!file) return;

  const reader = new FileReader();

  reader.onload = function (e) {
    try {
      trace = JSON.parse(e.target.result);

      if (!trace.cycles || !Array.isArray(trace.cycles)) {
        alert("Invalid trace file: expected a cycles array.");
        return;
      }

      currentIndex = 0;
      cycleSlider.min = 0;
      cycleSlider.max = trace.cycles.length - 1;
      cycleSlider.value = 0;

      render();
    } catch (error) {
      alert("Could not parse trace JSON.");
      console.error(error);
    }
  };

  reader.readAsText(file);
}

function previousCycle() {
  if (!trace) return;
  currentIndex = Math.max(0, currentIndex - 1);
  render();
}

function nextCycle() {
  if (!trace) return;
  currentIndex = Math.min(trace.cycles.length - 1, currentIndex + 1);
  render();
}

function play() {
  if (!trace || playTimer) return;

  playTimer = setInterval(() => {
    if (currentIndex >= trace.cycles.length - 1) {
      pause();
      return;
    }

    currentIndex++;
    render();
  }, 600);
}

function pause() {
  if (playTimer) {
    clearInterval(playTimer);
    playTimer = null;
  }
}

function render() {
  if (!trace) return;

  const cycle = trace.cycles[currentIndex];
  cycleSlider.value = currentIndex;

  cycleText.textContent = `Cycle ${cycle.cycle}`;
  pcText.textContent = `PC: ${cycle.pc}`;

  issuedText.textContent = cycle.issuedInstruction || "none";
  cdbText.textContent = cycle.cdbBroadcast || "none";
  commitText.textContent = cycle.commitEvent || "none";

  robHeadText.textContent = `ROB${cycle.rob.head}`;
  robTailText.textContent = `ROB${cycle.rob.tail}`;
  robCountText.textContent = cycle.rob.count;

  renderEvents(cycle.events || []);
  renderROB(cycle.rob.entries || []);
  renderActiveInstructions(cycle.activeInstructions || []);
  renderLSQ(cycle.lsq || []);
}

function renderEvents(events) {
  eventsList.innerHTML = "";

  if (events.length === 0) {
    const li = document.createElement("li");
    li.textContent = "No events this cycle.";
    li.className = "empty";
    eventsList.appendChild(li);
    return;
  }

  for (const event of events) {
    const li = document.createElement("li");
    li.textContent = event;
    eventsList.appendChild(li);
  }
}

function renderROB(entries) {
  robEntries.innerHTML = "";

  if (entries.length === 0) {
    robEntries.appendChild(emptyMessage("ROB is empty."));
    return;
  }

  for (const entry of entries) {
    const card = document.createElement("div");
    card.className = "card";

    const readyClass = entry.ready ? "ready" : "not-ready";
    const readyText = entry.ready ? "ready" : "not ready";

    card.innerHTML = `
      <div class="card-title">ROB${entry.robTag} | I${entry.instructionId}</div>
      <div>${escapeHtml(entry.rawText)}</div>
      <div class="card-row">
        <span>Status</span>
        <span class="badge ${readyClass}">${readyText}</span>
      </div>
      <div class="card-row">
        <span>Writes Register</span>
        <span>${entry.writesRegister ? "yes" : "no"}</span>
      </div>
      <div class="card-row">
        <span>Destination</span>
        <span>${entry.destinationRegister >= 0 ? "R" + entry.destinationRegister : "-"}</span>
      </div>
      <div class="card-row">
        <span>Value</span>
        <span>${entry.value}</span>
      </div>
      <div class="card-row">
        <span>Writes Memory</span>
        <span>${entry.writesMemory ? "yes" : "no"}</span>
      </div>
      <div class="card-row">
        <span>Memory</span>
        <span>${entry.writesMemory ? `Mem[${entry.memoryAddress}] = ${entry.memoryValue}` : "-"}</span>
      </div>
    `;

    robEntries.appendChild(card);
  }
}

function renderActiveInstructions(entries) {
  activeInstructions.innerHTML = "";

  if (entries.length === 0) {
    activeInstructions.appendChild(emptyMessage("No active instructions."));
    return;
  }

  for (const entry of entries) {
    const card = document.createElement("div");
    card.className = "card";

    const stateClass = entry.executing ? "executing" : "waiting";
    const stateText = entry.executing ? "executing" : "waiting";

    card.innerHTML = `
      <div class="card-title">I${entry.instructionId} | ROB${entry.robTag}</div>
      <div>${escapeHtml(entry.rawText)}</div>
      <div class="card-row">
        <span>State</span>
        <span class="badge ${stateClass}">${stateText}</span>
      </div>
      <div class="card-row">
        <span>Remaining</span>
        <span>${entry.remainingCycles}</span>
      </div>
      <div class="card-row">
        <span>Reason</span>
        <span>${escapeHtml(entry.waitingReason || "-")}</span>
      </div>
      <div class="card-row">
        <span>Vj / Vk</span>
        <span>${entry.vj} / ${entry.vk}</span>
      </div>
      <div class="card-row">
        <span>Qj / Qk</span>
        <span>${formatTag(entry.qj)} / ${formatTag(entry.qk)}</span>
      </div>
    `;

    activeInstructions.appendChild(card);
  }
}

function renderLSQ(entries) {
  lsqEntries.innerHTML = "";

  if (entries.length === 0) {
    lsqEntries.appendChild(emptyMessage("LSQ is empty."));
    return;
  }

  for (const entry of entries) {
    const card = document.createElement("div");
    card.className = "card";

    const type = entry.isLoad ? "Load" : "Store";

    card.innerHTML = `
      <div class="card-title">I${entry.instructionId} | ROB${entry.robTag}</div>
      <div>${escapeHtml(entry.rawText)}</div>
      <div class="card-row">
        <span>Type</span>
        <span>${type}</span>
      </div>
      <div class="card-row">
        <span>Address</span>
        <span>${entry.addressReady ? entry.address : "not ready"}</span>
      </div>
      <div class="card-row">
        <span>Value</span>
        <span>${entry.valueReady ? entry.value : "not ready"}</span>
      </div>
    `;

    lsqEntries.appendChild(card);
  }
}

function emptyMessage(text) {
  const div = document.createElement("div");
  div.className = "empty";
  div.textContent = text;
  return div;
}

function formatTag(tag) {
  return tag >= 0 ? `ROB${tag}` : "-";
}

function escapeHtml(value) {
  return String(value)
    .replaceAll("&", "&amp;")
    .replaceAll("<", "&lt;")
    .replaceAll(">", "&gt;")
    .replaceAll('"', "&quot;")
    .replaceAll("'", "&#039;");
}
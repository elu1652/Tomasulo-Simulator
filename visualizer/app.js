let trace = null;
let programLines = [];
let currentIndex = 0;
let playTimer = null;

// DOM references
const traceFileInput = document.getElementById("traceFile");
const programFileInput = document.getElementById("programFile");
const assemblyInput = document.getElementById("assemblyInput");
const predictorSelect = document.getElementById("predictorSelect");
const runSimulationBtn = document.getElementById("runSimulationBtn");
const runStatus = document.getElementById("runStatus");

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

// Supports both new HTML id="robTable" and old HTML id="robEntries"
const robTable =
  document.getElementById("robTable") ||
  document.getElementById("robEntries");

const lsqEntries = document.getElementById("lsqEntries");
const registerProducers = document.getElementById("registerProducers");
const branchPredictorSummary = document.getElementById("branchPredictorSummary");
const branchPredictorTable = document.getElementById("branchPredictorTable");
const registerState = document.getElementById("registerState");
const memoryState = document.getElementById("memoryState");
const instructionStatusTable = document.getElementById("instructionStatusTable");
const programListing = document.getElementById("programListing");

const intRSTable = document.getElementById("intRSTable");
const mulRSTable = document.getElementById("mulRSTable");
const loadBufferTable = document.getElementById("loadBufferTable");
const storeBufferTable = document.getElementById("storeBufferTable");

const programBox = document.getElementById("programBox");
const issueBox = document.getElementById("issueBox");
const rsBox = document.getElementById("rsBox");
const fuBox = document.getElementById("fuBox");
const cdbBox = document.getElementById("cdbBox");
const robBox = document.getElementById("robBox");
const lsqBox = document.getElementById("lsqBox");
const memoryBox = document.getElementById("memoryBox");
const commitBox = document.getElementById("commitBox");

const programBoxMain = document.getElementById("programBoxMain");
const programBoxSub = document.getElementById("programBoxSub");
const issueBoxMain = document.getElementById("issueBoxMain");
const issueBoxSub = document.getElementById("issueBoxSub");
const rsBoxMain = document.getElementById("rsBoxMain");
const fuBoxMain = document.getElementById("fuBoxMain");
const fuBoxSub = document.getElementById("fuBoxSub");
const cdbBoxMain = document.getElementById("cdbBoxMain");
const robBoxMain = document.getElementById("robBoxMain");
const robBoxSub = document.getElementById("robBoxSub");
const lsqBoxMain = document.getElementById("lsqBoxMain");
const memoryBoxMain = document.getElementById("memoryBoxMain");
const commitBoxMain = document.getElementById("commitBoxMain");

const arrowProgramIssue = document.getElementById("arrowProgramIssue");
const arrowIssueRS = document.getElementById("arrowIssueRS");
const arrowRSFU = document.getElementById("arrowRSFU");
const arrowFUCDB = document.getElementById("arrowFUCDB");
const arrowCDBROB = document.getElementById("arrowCDBROB");
const arrowRSLSQ = document.getElementById("arrowRSLSQ");
const arrowLSQMemory = document.getElementById("arrowLSQMemory");
const arrowROBCommit = document.getElementById("arrowROBCommit");

const componentBoxes = [
  programBox,
  issueBox,
  rsBox,
  fuBox,
  cdbBox,
  robBox,
  lsqBox,
  memoryBox,
  commitBox
].filter(Boolean);

const arrows = [
  arrowProgramIssue,
  arrowIssueRS,
  arrowRSFU,
  arrowFUCDB,
  arrowCDBROB,
  arrowRSLSQ,
  arrowLSQMemory,
  arrowROBCommit
].filter(Boolean);

// Event wiring
if (traceFileInput) {
  traceFileInput.addEventListener("change", handleTraceFile);
}

if (programFileInput) {
  programFileInput.addEventListener("change", handleProgramFile);
}

if (runSimulationBtn) {
  runSimulationBtn.addEventListener("click", runSimulationFromInput);
}

if (prevBtn) {
  prevBtn.addEventListener("click", previousCycle);
}

if (nextBtn) {
  nextBtn.addEventListener("click", nextCycle);
}

if (playBtn) {
  playBtn.addEventListener("click", play);
}

if (pauseBtn) {
  pauseBtn.addEventListener("click", pause);
}

if (cycleSlider) {
  cycleSlider.addEventListener("input", () => {
    if (!trace) return;
    currentIndex = Number(cycleSlider.value);
    render();
  });
}

document.addEventListener("keydown", (event) => {
  if (isEditableTarget(event.target)) return;
  if (!trace) return;

  if (event.key === "ArrowLeft") {
    previousCycle();
  } else if (event.key === "ArrowRight") {
    nextCycle();
  } else if (event.key === " ") {
    event.preventDefault();

    if (playTimer) {
      pause();
    } else {
      play();
    }
  }
});

// File loading / backend run
function handleTraceFile(event) {
  const file = event.target.files[0];
  if (!file) return;

  const reader = new FileReader();

  reader.onload = function (e) {
    try {
      const parsedTrace = JSON.parse(e.target.result);
      loadTrace(parsedTrace);
      setRunStatus("Loaded trace.json.", "success");
    } catch (error) {
      alert("Could not load trace file. Check the browser console for details.");
      console.error(error);
    }
  };

  reader.readAsText(file);
}

function handleProgramFile(event) {
  const file = event.target.files[0];
  if (!file) return;

  const reader = new FileReader();

  reader.onload = function (e) {
    const assemblyCode = e.target.result;
    programLines = parseProgramLines(assemblyCode);

    if (assemblyInput) {
      assemblyInput.value = assemblyCode;
    }

    render();
  };

  reader.readAsText(file);
}

async function runSimulationFromInput() {
  if (!assemblyInput) return;

  const assemblyCode = assemblyInput.value;

  if (!assemblyCode.trim()) {
    setRunStatus("Paste assembly code or load an .asm file first.", "error");
    return;
  }

  pause();
  setRunStatus("Running simulator...", "");
  setRunButtonDisabled(true);

  try {
    const response = await fetch("/run", {
      method: "POST",
      headers: {
        "Content-Type": "application/json"
      },
      body: JSON.stringify({
        code: assemblyCode,
        assembly: assemblyCode,
        predictor: predictorSelect ? predictorSelect.value : undefined
      })
    });

    const payload = await response.json();

    if (!response.ok) {
      throw new Error(payload.error || "Simulation failed.");
    }

    loadTrace(payload, parseProgramLines(assemblyCode));
    setRunStatus(`Simulation complete. Loaded ${payload.cycles.length} cycles.`, "success");
  } catch (error) {
    console.error(error);
    setRunStatus(error.message, "error");
  } finally {
    setRunButtonDisabled(false);
  }
}

function loadTrace(parsedTrace, nextProgramLines) {
  if (!parsedTrace.cycles || !Array.isArray(parsedTrace.cycles)) {
    throw new Error("Invalid trace file: expected a cycles array.");
  }

  trace = parsedTrace;

  if (Array.isArray(nextProgramLines)) {
    programLines = nextProgramLines;
  } else if (Array.isArray(trace.program)) {
    programLines = trace.program;
  }

  currentIndex = 0;

  if (cycleSlider) {
    cycleSlider.min = 0;
    cycleSlider.max = trace.cycles.length - 1;
    cycleSlider.value = 0;
  }

  render();
}

function parseProgramLines(text) {
  return text
    .split(/\r?\n/)
    .map((line) => line.trim())
    .filter((line) => line.length > 0)
    .filter((line) => !line.startsWith("#"))
    .filter((line) => !line.startsWith("//"))
    .filter((line) => !line.endsWith(":"));
}

// Cycle controls
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

// Main render function
function render() {
  if (!trace) return;

  const cycle = trace.cycles[currentIndex];
  const events = cycle.events || [];
  const rob = cycle.rob || { head: "-", tail: "-", count: 0, entries: [] };

  if (cycleSlider) {
    cycleSlider.value = currentIndex;
  }

  setText(cycleText, `Cycle ${cycle.cycle}`);
  setText(pcText, `PC: ${cycle.pc}`);

  setText(issuedText, cycle.issuedInstruction || "none");
  setText(cdbText, cycle.cdbBroadcast || "none");
  setText(commitText, cycle.commitEvent || "none");

  setText(robHeadText, `ROB${rob.head}`);
  setText(robTailText, `ROB${rob.tail}`);
  setText(robCountText, rob.count);

  renderProgram(cycle);
  renderDatapath(cycle, events);
  renderEvents(events);
  renderROB(rob.entries || []);
  renderRegisterProducers(cycle.registerProducers || []);
  renderBranchPredictions(cycle);
  renderReservationStations(cycle.activeInstructions || []);
  renderLSQ(cycle.lsq || []);
  renderRegisterState(cycle.registers);
  renderMemoryState(cycle.memory);
  renderInstructionStatus(cycle);
}

// Program / PC rendering
function renderProgram(cycle) {
  if (!programListing) return;

  programListing.innerHTML = "";

  if (programLines.length === 0) {
    const empty = document.createElement("div");
    empty.className = "program-line";
    empty.innerHTML = `
      <span class="program-line-number">-</span>
      <span>Load the .asm file to show program lines.</span>
    `;
    programListing.appendChild(empty);
    return;
  }

  const issuedIndex = getIssuedProgramIndex(cycle);

  for (let i = 0; i < programLines.length; i++) {
    const line = document.createElement("div");
    line.className = "program-line";

    if (i === cycle.pc) {
      line.classList.add("current-pc");
    }

    if (i === issuedIndex) {
      line.classList.add("issued-line");
    }

    line.innerHTML = `
      <span class="program-line-number">${i}</span>
      <span>${escapeHtml(programLines[i])}</span>
    `;

    programListing.appendChild(line);
  }
}

function findIssuedProgramIndex(issuedInstruction) {
  if (!issuedInstruction) return -1;

  const normalizedIssued = normalizeInstruction(issuedInstruction);

  for (let i = 0; i < programLines.length; i++) {
    if (normalizeInstruction(programLines[i]) === normalizedIssued) {
      return i;
    }
  }

  return -1;
}

function getIssuedProgramIndex(cycle) {
  if (!cycle.issuedInstruction) return -1;

  if (typeof cycle.pc === "number") {
    return cycle.pc - 1;
  }

  return findIssuedProgramIndex(cycle.issuedInstruction);
}

// Datapath rendering
function renderDatapath(cycle, events) {
  clearHighlights();

  const hasIssued = Boolean(cycle.issuedInstruction);
  const hasCDB = cycle.cdbBroadcast && cycle.cdbBroadcast !== "none";
  const hasCommit = Boolean(cycle.commitEvent);
  const hasStall = hasEvent(events, "Issue stalled");
  const hasExecutionStart = hasEvent(events, "Execution started");
  const hasExecutionComplete = hasEvent(events, "Execution complete");
  const hasMispredict = hasEvent(events, "Branch misprediction");
  const hasFlush = hasEvent(events, "Flushed");
  const hasBranch = hasEvent(events, "Branch resolved") || hasEvent(events, "Branch prediction");
  const hasMemoryInstruction = isMemoryInstruction(cycle.issuedInstruction);
  const hasLSQ = hasMemoryInstruction || hasEvent(events, "LSQ");
  const hasMemory =
    hasEvent(events, "Memory Commit") ||
    hasEvent(events, "Store result ready") ||
    hasEvent(events, "Load result ready");

  const activeCount = cycle.activeInstructions ? cycle.activeInstructions.length : 0;
  const rob = cycle.rob || { head: "-", tail: "-", count: 0 };
  const robCount = rob.count;
  const lsqCount = cycle.lsq ? cycle.lsq.length : 0;

  setText(programBoxMain, `PC: ${cycle.pc}`);
  setText(programBoxSub, getProgramLine(cycle.pc));

  setText(issueBoxMain, cycle.issuedInstruction || "none");
  setText(issueBoxSub, hasStall ? "issue stalled" : "instruction issue");

  setText(rsBoxMain, `${activeCount} active`);

  setText(
    fuBoxMain,
    hasExecutionStart
      ? getFirstEvent(events, "Execution started")
      : "INT / MUL / MEM"
  );

  setText(
    fuBoxSub,
    hasExecutionComplete ? "completed this cycle" : "execution"
  );

  setText(cdbBoxMain, hasCDB ? cycle.cdbBroadcast : "none");

  setText(robBoxMain, `${robCount} entries`);
  setText(robBoxSub, `head ROB${rob.head}, tail ROB${rob.tail}`);

  setText(lsqBoxMain, `${lsqCount} entries`);
  setText(memoryBoxMain, hasMemory ? "memory event" : "loads/stores");
  setText(commitBoxMain, cycle.commitEvent || "none");

  if (hasIssued) {
    activate(programBox, arrowProgramIssue, issueBox, arrowIssueRS, rsBox);
  }

  if (hasStall) {
    markStall(issueBox, arrowProgramIssue);
  }

  if (hasExecutionStart || hasExecutionComplete) {
    activate(rsBox, arrowRSFU, fuBox);
  }

  if (hasCDB) {
    activate(fuBox, arrowFUCDB, cdbBox, arrowCDBROB, robBox);
  }

  if (hasCommit) {
    markCommit(robBox, arrowROBCommit, commitBox);
  }

  if (hasBranch) {
    activate(programBox, issueBox);
  }

  if (hasMispredict || hasFlush) {
    markFlush(programBox, issueBox, rsBox, robBox);
  }

  if (hasLSQ) {
    activate(rsBox, arrowRSLSQ, lsqBox);
  }

  if (hasMemory) {
    activate(lsqBox, arrowLSQMemory, memoryBox);
  }
}

function clearHighlights() {
  for (const box of componentBoxes) {
    box.classList.remove("active", "stall", "flush", "mispredict", "commit-active");
  }

  for (const arrow of arrows) {
    arrow.classList.remove("active", "stall", "flush", "mispredict", "commit-active");
  }
}

function activate(...elements) {
  for (const element of elements) {
    if (element) {
      element.classList.add("active");
    }
  }
}

function markStall(...elements) {
  for (const element of elements) {
    if (element) {
      element.classList.add("stall");
    }
  }
}

function markFlush(...elements) {
  for (const element of elements) {
    if (element) {
      element.classList.add("flush");
    }
  }
}

function markCommit(...elements) {
  for (const element of elements) {
    if (element) {
      element.classList.add("commit-active");
    }
  }
}

// Event rendering
function renderEvents(events) {
  if (!eventsList) return;

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
    li.className = getEventClass(event);
    eventsList.appendChild(li);
  }
}

// ROB rendering
function renderROB(entries) {
  if (!robTable) {
    console.error("Missing ROB container. Expected id='robTable' or id='robEntries'.");
    return;
  }

  const capacity = getROBCapacity();

  let html = `
    <table class="rob-table">
      <thead>
        <tr>
          <th>Slot</th>
          <th>Markers</th>
          <th>Busy</th>
          <th>Instruction</th>
          <th>Ready</th>
          <th>Destination</th>
          <th>Value</th>
          <th>Memory Write</th>
        </tr>
      </thead>
      <tbody>
  `;

  for (let slot = 0; slot < capacity; slot++) {
    const entry = entries.find((e) => e.robTag === slot);
    const markers = getROBMarkers(slot);
    const markerClass = getROBMarkerClass(slot);

    if (!entry) {
      html += `
        <tr class="empty-row ${markerClass}">
          <td class="rob-slot">ROB${slot}</td>
          <td>${formatROBMarkers(markers)}</td>
          <td>no</td>
          <td>-</td>
          <td>-</td>
          <td>-</td>
          <td>-</td>
          <td>-</td>
        </tr>
      `;
      continue;
    }

    const rowClass = entry.ready ? "ready-row" : "not-ready-row";

    const destination =
      entry.writesRegister && entry.destinationRegister >= 0
        ? `R${entry.destinationRegister}`
        : "-";

    const memoryWrite = entry.writesMemory
      ? `Mem[${entry.memoryAddress}] = ${entry.memoryValue}`
      : "-";

    html += `
      <tr class="${rowClass} ${markerClass}">
        <td class="rob-slot">ROB${slot}</td>
        <td>${formatROBMarkers(markers)}</td>
        <td>yes</td>
        <td>
          <div class="rob-tag">I${entry.instructionId}</div>
          <div>${escapeHtml(entry.rawText)}</div>
        </td>
        <td>
          <span class="rob-ready-state ${entry.ready ? "ready" : "pending"}">
            ${entry.ready ? "ready" : "pending"}
          </span>
        </td>
        <td>${destination}</td>
        <td>${entry.value}</td>
        <td>${memoryWrite}</td>
      </tr>
    `;
  }

  html += `
      </tbody>
    </table>
  `;

  robTable.innerHTML = html;
}

function getROBCapacity() {
  if (!trace || !trace.cycles) {
    return 4;
  }

  let maxTag = 0;

  for (const cycle of trace.cycles) {
    const entries = cycle.rob?.entries || [];

    for (const entry of entries) {
      maxTag = Math.max(maxTag, entry.robTag);
    }

    if (typeof cycle.rob?.head === "number") {
      maxTag = Math.max(maxTag, cycle.rob.head);
    }

    if (typeof cycle.rob?.tail === "number") {
      maxTag = Math.max(maxTag, cycle.rob.tail);
    }
  }

  return maxTag + 1;
}

function getROBMarkers(slot) {
  if (!trace) {
    return "";
  }

  const cycle = trace.cycles[currentIndex];
  const markers = [];

  if (cycle.rob && cycle.rob.head === slot) {
    markers.push("HEAD");
  }

  if (cycle.rob && cycle.rob.tail === slot) {
    markers.push("TAIL");
  }

  return markers.join(" / ");
}

function formatROBMarkers(markerText) {
  if (!markerText) {
    return "-";
  }

  return markerText
    .split(" / ")
    .map((marker) => {
      const className = marker.toLowerCase();
      return `<span class="rob-marker ${className}">${marker}</span>`;
    })
    .join("");
}

function getROBMarkerClass(slot) {
  if (!trace) {
    return "";
  }

  const cycle = trace.cycles[currentIndex];
  let className = "";

  if (cycle.rob && cycle.rob.head === slot) {
    className += " head-row";
  }

  if (cycle.rob && cycle.rob.tail === slot) {
    className += " tail-row";
  }

  return className;
}

// Reservation station rendering
function renderReservationStations(activeEntries) {
  if (!intRSTable || !mulRSTable || !loadBufferTable || !storeBufferTable) {
    return;
  }

  const grouped = {
    INT: [],
    MUL: [],
    LOAD: [],
    STORE: []
  };

  for (const entry of activeEntries) {
    const type = inferRSType(entry.rawText);
    grouped[type].push(entry);
  }

  renderRSTable(intRSTable, grouped.INT, 2, "INT");
  renderRSTable(mulRSTable, grouped.MUL, 2, "MUL");
  renderRSTable(loadBufferTable, grouped.LOAD, 2, "LOAD");
  renderRSTable(storeBufferTable, grouped.STORE, 2, "STORE");
}

function renderRSTable(container, entries, capacity, type) {
  let html = `
    <table class="rs-table">
      <thead>
        <tr>
          <th>Slot</th>
          <th>Busy</th>
          <th>Instruction</th>
          <th>Op</th>
          <th>Vj</th>
          <th>Vk</th>
          <th>Qj</th>
          <th>Qk</th>
          <th>ROB</th>
          <th>Rem</th>
          <th>State</th>
        </tr>
      </thead>
      <tbody>
  `;

  for (let i = 0; i < capacity; i++) {
    const entry = entries[i];

    if (!entry) {
      html += `
        <tr class="empty-row">
          <td>${type}${i}</td>
          <td><span class="rs-status empty">empty</span></td>
          <td>-</td>
          <td>-</td>
          <td>-</td>
          <td>-</td>
          <td>-</td>
          <td>-</td>
          <td>-</td>
          <td>-</td>
          <td>empty</td>
        </tr>
      `;
      continue;
    }

    const rowClass = entry.executing ? "occupied-row executing-row" : "occupied-row waiting-row";
    const stateText = entry.executing ? "executing" : "waiting";
    const op = getOpcode(entry.rawText);

    html += `
      <tr class="${rowClass}">
        <td>${type}${i}</td>
        <td><span class="rs-status busy">busy</span></td>
        <td>${escapeHtml(entry.rawText)}</td>
        <td class="rs-op">${escapeHtml(op)}</td>
        <td>${entry.vj}</td>
        <td>${entry.vk}</td>
        <td class="rs-tag">${formatTag(entry.qj)}</td>
        <td class="rs-tag">${formatTag(entry.qk)}</td>
        <td class="rs-tag">ROB${entry.robTag}</td>
        <td>${entry.remainingCycles}</td>
        <td>${escapeHtml(entry.waitingReason || stateText)}</td>
      </tr>
    `;
  }

  html += `
      </tbody>
    </table>
  `;

  container.innerHTML = html;
}

function inferRSType(rawText) {
  const op = getOpcode(rawText);

  if (op === "MUL") {
    return "MUL";
  }

  if (op === "LD") {
    return "LOAD";
  }

  if (op === "SD") {
    return "STORE";
  }

  return "INT";
}

function getOpcode(rawText) {
  return String(rawText).trim().split(/\s+/)[0].toUpperCase();
}

function isMemoryInstruction(rawText) {
  const op = getOpcode(rawText);
  return op === "LD" || op === "SD" || op === "LOAD" || op === "STORE";
}

// LSQ rendering
function renderLSQ(entries) {
  if (!lsqEntries) return;

  lsqEntries.innerHTML = "";

  if (entries.length === 0) {
    lsqEntries.appendChild(emptyMessage("LSQ is empty."));
    return;
  }

  let html = `
    <table class="lsq-table">
      <thead>
        <tr>
          <th>Entry</th>
          <th>Type</th>
          <th>Instruction</th>
          <th>ROB</th>
          <th>Address Ready</th>
          <th>Address</th>
          <th>Value Ready</th>
          <th>Value</th>
        </tr>
      </thead>
      <tbody>
  `;

  entries.forEach((entry, index) => {
    const isLoad = Boolean(entry.isLoad);
    const isStore = Boolean(entry.isStore);
    const type = isLoad ? "Load" : isStore ? "Store" : "Unknown";
    const rowClasses = [
      isLoad ? "load-row" : "",
      isStore ? "store-row" : "",
      entry.addressReady ? "address-ready-row" : "address-not-ready-row",
      entry.valueReady ? "value-ready-row" : ""
    ].filter(Boolean).join(" ");

    html += `
      <tr class="${rowClasses}">
        <td class="rs-tag">LSQ${index}</td>
        <td><span class="lsq-type ${type.toLowerCase()}">${type}</span></td>
        <td>${escapeHtml(entry.rawText || "-")}</td>
        <td class="rs-tag">${formatTag(entry.robTag)}</td>
        <td>${formatReady(entry.addressReady)}</td>
        <td>${entry.addressReady ? entry.address : "-"}</td>
        <td>${formatReady(entry.valueReady)}</td>
        <td>${entry.valueReady ? entry.value : "-"}</td>
      </tr>
    `;
  });

  html += `
      </tbody>
    </table>
  `;

  lsqEntries.innerHTML = html;
}

// Register producer rendering
function renderRegisterProducers(producers) {
  if (!registerProducers) return;

  registerProducers.innerHTML = "";

  if (!Array.isArray(producers) || producers.length === 0) {
    registerProducers.appendChild(emptyMessage("No active register producers."));
    return;
  }

  const previousCycle = currentIndex > 0 ? trace.cycles[currentIndex - 1] : null;
  const previousProducers = new Map(
    (previousCycle?.registerProducers || [])
      .map((producer) => [getProducerRegister(producer), producer.robTag])
  );

  const sortedProducers = [...producers].sort(
    (left, right) => getProducerRegister(left) - getProducerRegister(right)
  );

  let html = `
    <table class="producer-table">
      <thead>
        <tr>
          <th>Register</th>
          <th>Producer ROB</th>
        </tr>
      </thead>
      <tbody>
  `;

  for (const producer of sortedProducers) {
    const registerNumber = getProducerRegister(producer);
    const changed = previousCycle
      ? previousProducers.get(registerNumber) !== producer.robTag
      : false;

    html += `
      <tr class="${changed ? "changed-row" : ""}">
        <td class="rs-tag">R${registerNumber}</td>
        <td class="rs-tag">${formatTag(producer.robTag)}</td>
      </tr>
    `;
  }

  html += `
      </tbody>
    </table>
  `;

  registerProducers.innerHTML = html;
}

function getProducerRegister(producer) {
  return producer.register ?? producer.registerNumber ?? -1;
}

// Branch predictor rendering
function renderBranchPredictions(cycle) {
  if (!branchPredictorSummary || !branchPredictorTable) return;

  branchPredictorSummary.innerHTML = "";
  branchPredictorTable.innerHTML = "";

  if (!Object.prototype.hasOwnProperty.call(cycle, "branchPredictions")) {
    branchPredictorSummary.appendChild(emptyMessage("Branch prediction data not available."));
    return;
  }

  const predictions = Array.isArray(cycle.branchPredictions)
    ? cycle.branchPredictions
    : [];

  renderBranchSummary(predictions, cycle.predictorType);

  if (predictions.length === 0) {
    branchPredictorTable.appendChild(emptyMessage("No branch predictions issued yet."));
    return;
  }

  let html = `
    <table class="branch-table">
      <thead>
        <tr>
          <th>PC</th>
          <th>Instruction</th>
          <th>Predictor</th>
          <th>State Before</th>
          <th>Prediction</th>
          <th>Actual</th>
          <th>Correct?</th>
          <th>State After</th>
          <th>Target PC</th>
          <th>Fallthrough PC</th>
        </tr>
      </thead>
      <tbody>
  `;

  for (const prediction of predictions) {
    const resolved = Boolean(prediction.branchResolved);
    const correct = resolved && Boolean(prediction.predictionCorrect);
    const rowClass = resolved
      ? correct ? "branch-correct-row" : "branch-miss-row"
      : "branch-pending-row";

    html += `
      <tr class="${rowClass}">
        <td class="rs-tag">${formatNullableNumber(prediction.pc)}</td>
        <td>${escapeHtml(prediction.instruction || "-")}</td>
        <td>${escapeHtml(prediction.predictorType || cycle.predictorType || "-")}</td>
        <td>${escapeHtml(formatStateText(prediction.stateBeforeText, prediction.stateBefore))}</td>
        <td>${formatDirection(prediction.predictedTaken)}</td>
        <td>${resolved ? formatDirection(prediction.actualTaken) : "pending"}</td>
        <td>${resolved ? (correct ? "yes" : "no") : "pending"}</td>
        <td>${escapeHtml(resolved ? formatStateText(prediction.stateAfterText, prediction.stateAfter) : "pending")}</td>
        <td class="rs-tag">${formatNullableNumber(prediction.targetPc)}</td>
        <td class="rs-tag">${formatNullableNumber(prediction.fallthroughPc)}</td>
      </tr>
    `;
  }

  html += `
      </tbody>
    </table>
  `;

  branchPredictorTable.innerHTML = html;
}

function renderBranchSummary(predictions, predictorType) {
  const resolved = predictions.filter((prediction) => prediction.branchResolved);
  const correct = resolved.filter((prediction) => prediction.predictionCorrect).length;
  const misses = resolved.length - correct;
  const accuracy = resolved.length > 0
    ? `${((100 * correct) / resolved.length).toFixed(1)}%`
    : "-";

  branchPredictorSummary.innerHTML = `
    <div class="summary-row">
      <span>Predictor</span>
      <strong>${escapeHtml(predictorType || "-")}</strong>
    </div>
    <div class="summary-row">
      <span>Resolved</span>
      <strong>${resolved.length}</strong>
    </div>
    <div class="summary-row">
      <span>Correct</span>
      <strong>${correct}</strong>
    </div>
    <div class="summary-row">
      <span>Mispredictions</span>
      <strong>${misses}</strong>
    </div>
    <div class="summary-row">
      <span>Accuracy</span>
      <strong>${accuracy}</strong>
    </div>
  `;
}

function formatDirection(taken) {
  return taken ? "taken" : "not taken";
}

function formatStateText(text, state) {
  if (text) {
    return state >= 0 ? `${state} ${text}` : text;
  }

  return state >= 0 ? String(state) : "N/A";
}

function formatNullableNumber(value) {
  return typeof value === "number" && value >= 0 ? value : "-";
}

// Register and memory rendering
function renderRegisterState(registers) {
  renderStateTable(registerState, registers, "R", "Register state not available");
}

function renderMemoryState(memory) {
  renderStateTable(memoryState, memory, "Mem", "Memory state not available");
}

function renderStateTable(container, values, labelPrefix, unavailableText) {
  if (!container) return;

  container.innerHTML = "";

  if (!Array.isArray(values)) {
    container.appendChild(emptyMessage(unavailableText));
    return;
  }

  const previousCycle = currentIndex > 0 ? trace.cycles[currentIndex - 1] : null;
  const previousValues = labelPrefix === "R"
    ? previousCycle?.registers
    : previousCycle?.memory;
  const displayValues = values.slice(0, 32);

  while (displayValues.length < 32) {
    displayValues.push(0);
  }

  const perRow = 4;
  let html = '<table class="state-table"><tbody>';

  for (let rowStart = 0; rowStart < 32; rowStart += perRow) {
    html += "<tr>";

    for (let offset = 0; offset < perRow; offset++) {
      const index = rowStart + offset;
      const value = displayValues[index];
      const changed = Array.isArray(previousValues) && previousValues[index] !== value;
      const label = labelPrefix === "R" ? `R${index}` : `Mem[${index}]`;

      html += `
        <th>${label}</th>
        <td class="${changed ? "changed-cell" : ""}">${value}</td>
      `;
    }

    html += "</tr>";
  }

  html += "</tbody></table>";
  container.innerHTML = html;
}

// Instruction status timeline rendering
function renderInstructionStatus(cycle) {
  if (!instructionStatusTable) return;

  instructionStatusTable.innerHTML = "";

  if (!Array.isArray(trace.instructionStatus)) {
    instructionStatusTable.appendChild(
      emptyMessage("Instruction status data not available.")
    );
    return;
  }

  const currentCycle = typeof cycle.cycle === "number"
    ? cycle.cycle
    : currentIndex + 1;

  const visibleEntries = trace.instructionStatus.filter((entry) => {
    const issueCycle = getStatusCycle(entry, "issueCycle");
    return issueCycle >= 0 && issueCycle <= currentCycle;
  });

  if (visibleEntries.length === 0) {
    instructionStatusTable.appendChild(emptyMessage("No instructions issued yet."));
    return;
  }

  let html = `
    <table class="status-table">
      <thead>
        <tr>
          <th>ID</th>
          <th>PC</th>
          <th>Instruction</th>
          <th>Issue</th>
          <th>Exec Start</th>
          <th>Exec End</th>
          <th>Write CDB</th>
          <th>Commit</th>
          <th>Flush</th>
        </tr>
      </thead>
      <tbody>
  `;

  for (const entry of visibleEntries) {
    const committed = isStageVisible(getStatusCycle(entry, "commitCycle"), currentCycle);
    const flushed = isFlushVisible(entry, currentCycle);
    const currentRow = hasCurrentInstructionStatusEvent(entry, currentCycle);
    const rowClasses = [
      committed ? "status-committed-row" : "",
      flushed ? "status-flushed-row" : "",
      currentRow ? "status-current-row" : ""
    ].filter(Boolean).join(" ");

    html += `
      <tr class="${rowClasses}">
        <td class="rs-tag">I${entry.instructionId}</td>
        <td class="rs-tag">${formatNullableNumber(entry.pc)}</td>
        <td>${escapeHtml(entry.rawText || "-")}</td>
        ${renderStatusCycleCell(entry, "issueCycle", currentCycle, "issue-stage-cell")}
        ${renderStatusCycleCell(entry, "execStartCycle", currentCycle, "exec-stage-cell")}
        ${renderStatusCycleCell(entry, "execEndCycle", currentCycle, "exec-stage-cell")}
        ${renderStatusCycleCell(entry, "writebackCycle", currentCycle, "wb-stage-cell")}
        ${renderStatusCycleCell(entry, "commitCycle", currentCycle, "commit-stage-cell")}
        ${renderFlushStatusCell(entry, currentCycle)}
      </tr>
    `;
  }

  html += `
      </tbody>
    </table>
  `;

  instructionStatusTable.innerHTML = html;
}

function renderStatusCycleCell(entry, fieldName, currentCycle, stageClass) {
  const cycle = getStatusCycle(entry, fieldName);

  if (cycle < 0) {
    return '<td class="status-missing-cell">-</td>';
  }

  if (cycle > currentCycle) {
    return `<td class="status-pending-cell ${stageClass}"></td>`;
  }

  const currentClass = cycle === currentCycle ? "current-stage-cell" : "";
  return `<td class="${stageClass} ${currentClass}">${cycle}</td>`;
}

function renderFlushStatusCell(entry, currentCycle) {
  if (!entry.flushed) {
    const commitCycle = getStatusCycle(entry, "commitCycle");
    const content = commitCycle >= 0 && commitCycle <= currentCycle ? "no" : "";
    const cellClass = content ? "status-no-flush-cell" : "status-pending-cell";
    return `<td class="${cellClass}">${content}</td>`;
  }

  const flushCycle = getStatusCycle(entry, "flushCycle");

  if (flushCycle >= 0 && flushCycle > currentCycle) {
    return '<td class="flush-stage-cell status-pending-cell"></td>';
  }

  const currentClass = flushCycle === currentCycle ? "current-stage-cell" : "";
  return `<td class="flush-stage-cell ${currentClass}">yes</td>`;
}

function getStatusCycle(entry, fieldName) {
  const value = entry[fieldName];
  return typeof value === "number" ? value : -1;
}

function isStageVisible(cycle, currentCycle) {
  return cycle >= 0 && cycle <= currentCycle;
}

function isFlushVisible(entry, currentCycle) {
  if (!entry.flushed) return false;

  const flushCycle = getStatusCycle(entry, "flushCycle");
  return flushCycle < 0 || flushCycle <= currentCycle;
}

function hasCurrentInstructionStatusEvent(entry, currentCycle) {
  const timingFields = [
    "issueCycle",
    "execStartCycle",
    "execEndCycle",
    "writebackCycle",
    "commitCycle",
    "flushCycle"
  ];

  return timingFields.some((fieldName) => {
    return getStatusCycle(entry, fieldName) === currentCycle;
  });
}

// Helper functions
function emptyMessage(text) {
  const div = document.createElement("div");
  div.className = "empty";
  div.textContent = text;
  return div;
}

function formatTag(tag) {
  return tag >= 0 ? `ROB${tag}` : "-";
}

function formatReady(isReady) {
  return `<span class="ready-pill ${isReady ? "ready" : "not-ready"}">${isReady ? "yes" : "no"}</span>`;
}

function hasEvent(events, text) {
  return events.some((event) => event.includes(text));
}

function getFirstEvent(events, text) {
  return events.find((event) => event.includes(text)) || "";
}

function getEventClass(event) {
  if (event.includes("Issue stalled")) return "event-stall";
  if (event.includes("CDB Broadcast")) return "event-cdb";
  if (event.includes("Committed")) return "event-commit";
  if (event.includes("Flushed")) return "event-flush";
  if (event.includes("misprediction")) return "event-mispredict";
  if (event.includes("Execution")) return "event-execute";
  if (event.includes("Branch")) return "event-branch";
  return "";
}

function getProgramLine(pc) {
  if (programLines.length === 0) {
    return "load .asm file";
  }

  if (pc < 0 || pc >= programLines.length) {
    return "program complete";
  }

  return programLines[pc];
}

function normalizeInstruction(text) {
  return String(text)
    .replace(/\/\/.*$/g, "")
    .replace(/#.*$/g, "")
    .replace(/\s+/g, " ")
    .trim()
    .toUpperCase();
}

function setText(element, value) {
  if (element) {
    element.textContent = value;
  }
}

function setRunStatus(message, state) {
  if (!runStatus) return;

  runStatus.textContent = message;
  runStatus.classList.remove("success", "error");

  if (state) {
    runStatus.classList.add(state);
  }
}

function setRunButtonDisabled(disabled) {
  if (runSimulationBtn) {
    runSimulationBtn.disabled = disabled;
  }
}

function isEditableTarget(target) {
  if (!target) return false;

  const tagName = target.tagName ? target.tagName.toLowerCase() : "";
  return tagName === "input" || tagName === "textarea" || target.isContentEditable;
}

function escapeHtml(value) {
  return String(value)
    .replaceAll("&", "&amp;")
    .replaceAll("<", "&lt;")
    .replaceAll(">", "&gt;")
    .replaceAll('"', "&quot;")
    .replaceAll("'", "&#039;");
}

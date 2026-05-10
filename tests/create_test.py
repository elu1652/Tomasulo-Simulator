#!/usr/bin/env python3

import tkinter as tk
from tkinter import messagebox
from pathlib import Path


ROOT = Path(__file__).resolve().parent.parent
TEST_DIR = ROOT / "tests"


class TestFileGenerator(tk.Tk):
    def __init__(self):
        super().__init__()

        self.title("Tomasulo Test File Generator")
        self.geometry("900x700")

        self.create_widgets()

    def create_widgets(self):
        # File name
        tk.Label(self, text="File name (example: basic_arithmetic.asm)").pack(anchor="w", padx=10, pady=(10, 0))
        self.filename_entry = tk.Entry(self, width=80)
        self.filename_entry.pack(fill="x", padx=10)

        # Test title
        tk.Label(self, text="Test title").pack(anchor="w", padx=10, pady=(8, 0))
        self.title_entry = tk.Entry(self, width=80)
        self.title_entry.pack(fill="x", padx=10)

        # Description
        tk.Label(self, text="Description").pack(anchor="w", padx=10, pady=(8, 0))
        self.description_text = tk.Text(self, height=4, width=80)
        self.description_text.pack(fill="x", padx=10)

        # Expected registers
        tk.Label(self, text="Expected registers, one per line (example: R1 7)").pack(anchor="w", padx=10, pady=(8, 0))
        self.expected_regs_text = tk.Text(self, height=4, width=80)
        self.expected_regs_text.pack(fill="x", padx=10)

        # Expected memory
        tk.Label(self, text="Expected memory, one per line (example: 8 7)").pack(anchor="w", padx=10, pady=(8, 0))
        self.expected_mem_text = tk.Text(self, height=3, width=80)
        self.expected_mem_text.pack(fill="x", padx=10)

        # Expected commit counts
        tk.Label(
            self,
            text="Expected commit counts, one per line (example: ADD R2, R1, R3 | 1)"
        ).pack(anchor="w", padx=10, pady=(8, 0))

        self.expected_commit_text = tk.Text(self, height=3, width=80)
        self.expected_commit_text.pack(fill="x", padx=10)

        # Assembly code
        tk.Label(self, text="Assembly code").pack(anchor="w", padx=10, pady=(8, 0))
        self.code_text = tk.Text(self, height=8, width=80)
        self.code_text.pack(fill="both", expand=False, padx=10)

        # Buttons
        button_frame = tk.Frame(self)
        button_frame.pack(fill="x", padx=10, pady=10)

        tk.Button(button_frame, text="Preview", command=self.preview_file).pack(side="left")
        tk.Button(button_frame, text="Create Test File", command=self.create_file).pack(side="left", padx=10)
        tk.Button(button_frame, text="Clear", command=self.clear_fields).pack(side="left")

    def get_clean_lines(self, text_widget):
        raw = text_widget.get("1.0", "end").splitlines()
        return [line.rstrip() for line in raw if line.strip()]

    def generate_content(self):
        filename = self.filename_entry.get().strip()
        test_title = self.title_entry.get().strip()

        description_lines = self.get_clean_lines(self.description_text)
        expected_regs = self.get_clean_lines(self.expected_regs_text)
        expected_mem = self.get_clean_lines(self.expected_mem_text)
        expected_commits = self.get_clean_lines(self.expected_commit_text)
        code_lines = self.get_clean_lines(self.code_text)

        if not filename:
            raise ValueError("File name cannot be empty.")

        if not filename.endswith(".asm"):
            filename += ".asm"

        output = []

        output.append(f"# {filename}")

        if test_title:
            output.append("#")
            output.append(f"# Test: {test_title}")

        if description_lines:
            output.append("#")
            for line in description_lines:
                output.append(f"# {line}")

        if expected_regs or expected_mem or expected_commits:
            output.append("#")
            output.append("# Expected final state:")

        for line in expected_regs:
            parts = line.split()

            if len(parts) != 2:
                raise ValueError(f"Invalid expected register line: {line}")

            reg, value = parts

            if not reg.upper().startswith("R"):
                raise ValueError(f"Register must look like R1, R2, etc: {line}")

            output.append(f"# EXPECT_REG {reg.upper()} {value}")

        for line in expected_mem:
            parts = line.split()

            if len(parts) != 2:
                raise ValueError(f"Invalid expected memory line: {line}")

            addr, value = parts
            output.append(f"# EXPECT_MEM {addr} {value}")

        for line in expected_commits:
            if "|" not in line:
                raise ValueError(
                    f"Invalid expected commit count line: {line}\n"
                    "Use format: ADD R2, R1, R3 | 1"
                )

            instr, count = line.rsplit("|", 1)
            instr = instr.strip()
            count = count.strip()

            if not instr:
                raise ValueError(f"Instruction cannot be empty: {line}")

            if not count.isdigit():
                raise ValueError(f"Commit count must be a non-negative integer: {line}")

            output.append(f"# EXPECT_COMMIT_COUNT {instr} {count}")

        output.append("")

        if not code_lines:
            raise ValueError("Assembly code cannot be empty.")

        output.extend(code_lines)
        output.append("")

        return filename, "\n".join(output)

    def preview_file(self):
        try:
            filename, content = self.generate_content()
        except ValueError as error:
            messagebox.showerror("Error", str(error))
            return

        preview = tk.Toplevel(self)
        preview.title(f"Preview: {filename}")
        preview.geometry("800x600")

        text = tk.Text(preview, wrap="none")
        text.pack(fill="both", expand=True)
        text.insert("1.0", content)
        text.config(state="disabled")

    def create_file(self):
        try:
            filename, content = self.generate_content()
        except ValueError as error:
            messagebox.showerror("Error", str(error))
            return

        TEST_DIR.mkdir(exist_ok=True)

        output_path = TEST_DIR / filename

        if output_path.exists():
            confirm = messagebox.askyesno(
                "Overwrite file?",
                f"{output_path} already exists.\n\nOverwrite it?"
            )

            if not confirm:
                return

        output_path.write_text(content)

        messagebox.showinfo(
            "Success",
            f"Created test file:\n{output_path}"
        )

    def clear_fields(self):
        self.filename_entry.delete(0, "end")
        self.title_entry.delete(0, "end")
        self.description_text.delete("1.0", "end")
        self.expected_regs_text.delete("1.0", "end")
        self.expected_mem_text.delete("1.0", "end")
        self.expected_commit_text.delete("1.0", "end")
        self.code_text.delete("1.0", "end")


if __name__ == "__main__":
    app = TestFileGenerator()
    app.mainloop()
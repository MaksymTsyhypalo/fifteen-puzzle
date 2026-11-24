import sys
import tkinter as tk
from tkinter import filedialog, messagebox

class FifteenPuzzleViewer:
    def __init__(self, root, board_file=None, sol_file=None):
        self.root = root
        self.root.title("15-Puzzle Step-by-Step Solution Viewer")
        self.root.geometry("520x640")
        self.root.configure(bg="#1e1e2e")

        self.R = 4
        self.C = 4
        self.states = []
        self.moves = ""
        self.current_step = 0
        self.playing = False

        self._build_ui()

        if board_file and sol_file:
            self.load_files(board_file, sol_file)

    def _build_ui(self):

        top_frame = tk.Frame(self.root, bg="#1e1e2e", pady=10)
        top_frame.pack(fill=tk.X)

        tk.Button(top_frame, text="Load Puzzle & Solution", command=self.prompt_files, bg="#438bff", fg="#11111b", font=("Helvetica", 10, "bold"), padx=10).pack()

        self.status_lbl = tk.Label(self.root, text="Step: 0 / 0 | Next Move: None", bg="#1e1e2e", fg="#ecf1ff", font=("Helvetica", 13, "bold"))
        self.status_lbl.pack(pady=5)

        self.canvas = tk.Canvas(self.root, width=400, height=400, bg="#40404C", highlightthickness=0)
        self.canvas.pack(pady=10)

        nav_frame = tk.Frame(self.root, bg="#1e1e2e", pady=10)
        nav_frame.pack()

        btn_style = {"bg": "#45475a", "fg": "#cdd6f4", "font": ("Helvetica", 11, "bold"), "width": 8}
        tk.Button(nav_frame, text="|< Start", command=lambda: self.jump_to(0), **btn_style).grid(row=0, column=0, padx=4)
        tk.Button(nav_frame, text="< Prev", command=self.prev_step, **btn_style).grid(row=0, column=1, padx=4)
        self.play_btn = tk.Button(nav_frame, text="Play", command=self.toggle_play, bg="#63875f", fg="#11111b", font=("Helvetica", 11, "bold"), width=8)
        self.play_btn.grid(row=0, column=2, padx=4)
        tk.Button(nav_frame, text="Next >", command=self.next_step, **btn_style).grid(row=0, column=3, padx=4)
        tk.Button(nav_frame, text="End >|", command=lambda: self.jump_to(len(self.states)-1), **btn_style).grid(row=0, column=4, padx=4)

        jump_frame = tk.Frame(self.root, bg="#1e1e2e", pady=10)
        jump_frame.pack(fill=tk.X, padx=40)

        self.slider = tk.Scale(jump_frame, from_=0, to=0, orient=tk.HORIZONTAL, command=self._on_slider, bg="#1e1e2e", fg="#cdd6f4", highlightthickness=0, troughcolor="#45475a")
        self.slider.pack(fill=tk.X)

        entry_row = tk.Frame(jump_frame, bg="#1e1e2e", pady=5)
        entry_row.pack()
        tk.Label(entry_row, text="Jump to step:", bg="#1e1e2e", fg="#eff3ff").pack(side=tk.LEFT)
        self.jump_entry = tk.Entry(entry_row, width=6, justify="center")
        self.jump_entry.pack(side=tk.LEFT, padx=5)
        tk.Button(entry_row, text="Go", command=self._on_jump_entry, bg="#f4fa87", fg="#1e1e2e").pack(side=tk.LEFT)

        self.root.bind("<Left>", lambda e: self.prev_step())
        self.root.bind("<Right>", lambda e: self.next_step())
        self.root.bind("<space>", lambda e: self.toggle_play())

    def prompt_files(self):
        b_path = filedialog.askopenfilename(title="Select Initial Puzzle File")
        if not b_path:
            return
        s_path = filedialog.askopenfilename(title="Select Solution Output File")
        if not s_path:
            return
        self.load_files(b_path, s_path)

    def load_files(self, board_path, sol_path):
        try:
            with open(board_path, "r") as f:
                tokens = f.read().split()
            self.R, self.C = int(tokens[0]), int(tokens[1])
            board = [int(x) for x in tokens[2:2 + self.R * self.C]]

            with open(sol_path, "r") as f:
                lines = [line.strip() for line in f.readlines()]
            length = int(lines[0])
            if length == -1:
                messagebox.showinfo("Unsolved", "The solver reported no solution (-1).")
                self.moves = ""
            else:
                self.moves = lines[1] if len(lines) > 1 else ""

            self._precompute_states(board, self.moves)
            self.slider.config(to=len(self.states) - 1)
            self.jump_to(0)
        except Exception as e:
            messagebox.showerror("Error", f"Failed to load files:\n{e}")

    def _precompute_states(self, initial_board, moves):
        self.states = [tuple(initial_board)]
        curr = list(initial_board)
        for idx, m in enumerate(moves):
            z = curr.index(0)
            zr, zc = divmod(z, self.C)
            nr, nc = zr, zc
            if m == 'L': nc += 1
            elif m == 'R': nc -= 1
            elif m == 'U': nr += 1
            elif m == 'D': nr -= 1
            else:
                raise ValueError(f"Invalid move '{m}' at index {idx}")

            if not (0 <= nr < self.R and 0 <= nc < self.C):
                raise ValueError(f"Out-of-bounds move '{m}' at step {idx + 1}")

            nz = nr * self.C + nc
            curr[z], curr[nz] = curr[nz], curr[z]
            self.states.append(tuple(curr))

    def draw_board(self):
        self.canvas.delete("all")
        if not self.states:
            return
        state = self.states[self.current_step]
        w = 400 / self.C
        h = 400 / self.R

        for i, val in enumerate(state):
            r, c = divmod(i, self.C)
            x0, y0 = c * w + 4, r * h + 4
            x1, y1 = (c + 1) * w - 4, (r + 1) * h - 4

            if val == 0:
                self.canvas.create_rectangle(x0, y0, x1, y1, fill="#181825", outline="")

            else:
                in_place = (val == i + 1)
                fill_color = "#a6e3a1" if in_place else "#89b4fa"
                self.canvas.create_rectangle(x0, y0, x1, y1, fill=fill_color, outline="#cdd6f4", width=2)
                self.canvas.create_text((x0 + x1) / 2, (y0 + y1) / 2, text=str(val), font=("Helvetica", 22, "bold"), fill="#11111b")

        next_m = self.moves[self.current_step] if self.current_step < len(self.moves) else "SOLVED"
        self.status_lbl.config(text=f"Step: {self.current_step} / {len(self.states)-1} | Next Move: {next_m}")

    def jump_to(self, step):
        if not self.states:
            return
        self.current_step = max(0, min(step, len(self.states) - 1))
        self.slider.set(self.current_step)
        self.draw_board()

    def next_step(self):
        if self.current_step < len(self.states) - 1:
            self.jump_to(self.current_step + 1)
        else:
            self.playing = False
            self.play_btn.config(text="Play")

    def prev_step(self):
        if self.current_step > 0:
            self.jump_to(self.current_step - 1)

    def _on_slider(self, val):
        self.current_step = int(val)
        self.draw_board()

    def _on_jump_entry(self):
        try:
            self.jump_to(int(self.jump_entry.get()))
        except ValueError:
            pass

    def toggle_play(self):
        self.playing = not self.playing
        self.play_btn.config(text="Pause" if self.playing else "Play")
        if self.playing:
            self._auto_advance()

    def _auto_advance(self):
        if self.playing and self.current_step < len(self.states) - 1:
            self.next_step()
            self.root.after(250, self._auto_advance)
        else:
            self.playing = False
            self.play_btn.config(text="Play")

if __name__ == "__main__":
    root = tk.Tk()
    b_arg = sys.argv[1] if len(sys.argv) > 1 else None
    s_arg = sys.argv[2] if len(sys.argv) > 2 else None
    app = FifteenPuzzleViewer(root, b_arg, s_arg)
    root.mainloop()

    

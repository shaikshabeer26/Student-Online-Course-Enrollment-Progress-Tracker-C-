#include<stdio.h>
#include<stdlib.h>
#include<string.h>

#define MAX_SUBJECTS 5
#define MAX_NAME 50
#define MAX_STUDENTS 50
#define MAX_COURSES 8

/* ===== Subject & Job templates (no progress here) ===== */
typedef struct {
    char name[50];
    char syllabusFile[50];
} SubjectDef;

typedef struct {
    char jobName[50];
    int subjectCount;
    SubjectDef subjects[MAX_SUBJECTS];
} Job;

/* ===== Per-student enrolled course (has progress) ===== */
typedef struct {
    char jobName[50];
    int subjectCount;
    struct {
        char name[50];
        char syllabusFile[50];
        int progress; /* 0-100 */
    } subjects[MAX_SUBJECTS];
} Course;

/* ===== Student ===== */
typedef struct {
    char name[MAX_NAME];
    int courseCount;
    Course courses[MAX_COURSES];
} Student;

/* ===== Jobs for 10th & 12th ===== */
static Job jobs10th[] = {
    {"SSC MTS", 3, {
        {"Maths", "maths.txt"},
        {"English", "english.txt"},
        {"General Knowledge", "gk.txt"}
    }},
    {"Constable", 2, {
        {"General Knowledge", "gk.txt"},
        {"Reasoning", "reasoning.txt"}
    }}
};
static int jobs10thCount = 2;

static Job jobs12th[] = {
    {"SSC CHSL", 3, {
        {"Maths", "maths.txt"},
        {"English", "english.txt"},
        {"General Awareness", "ga.txt"}
    }},
    {"Data Entry Operator", 2, {
        {"English", "english.txt"},
        {"Computer Basics", "computer.txt"}
    }}
};
static int jobs12thCount = 2;

/* ===== Simple in-memory DB ===== */
static Student students[MAX_STUDENTS];
static int studentCount = 0;

/* ===== Utilities ===== */
void flushLine(void) {
    int c;
    while ((c = getchar()) != '\n' && c != EOF) { /* discard */ }
}

int readIntInRange(const char *prompt, int lo, int hi) {
    int x, ok = 0;
    do {
        printf("%s", prompt);
        if (scanf("%d", &x) == 1 && x >= lo && x <= hi) {
            ok = 1;
        } else {
            printf("Invalid input. Please enter a number between %d and %d.\n", lo, hi);
        }
        flushLine();
    } while (!ok);
    return x;
}

void readLine(const char *prompt, char *buf, int buflen) {
    printf("%s", prompt);
    if (fgets(buf, buflen, stdin)) {
        size_t n = strlen(buf);
        if (n && buf[n-1] == '\n') buf[n-1] = '\0';
    } else {
        buf[0] = '\0';
    }
}

/* ===== Syllabus viewer ===== */
void showSyllabusFile(const char *filename) {
    FILE *f = fopen(filename, "r");
    if (!f) {
        printf("Could not open file: %s\n", filename);
        return;
    }
    char line[256];
    while (fgets(line, sizeof(line), f)) {
        fputs(line, stdout);
    }
    fclose(f);
}
/* ===== Course creation from Job template ===== */
void makeCourseFromJob(const Job *job, Course *courseOut) {
    int i;
    strcpy(courseOut->jobName, job->jobName);
    courseOut->subjectCount = job->subjectCount;
    for (i = 0; i < job->subjectCount; i++) {
        strcpy(courseOut->subjects[i].name, job->subjects[i].name);
        strcpy(courseOut->subjects[i].syllabusFile, job->subjects[i].syllabusFile);
        courseOut->subjects[i].progress = 0;
    }
}

/* ===== Find or add student ===== */
int findStudentIndexByName(const char *name) {
    int i;
    for (i = 0; i < studentCount; i++) {
        if (strcmp(students[i].name, name) == 0) return i;
    }
    return -1;
}

int addNewStudent(const char *name) {
    if (studentCount >= MAX_STUDENTS) return -1;
    strcpy(students[studentCount].name, name);
    students[studentCount].courseCount = 0;
    return studentCount++;
}

/* ===== Choose qualification & job ===== */
int chooseQualification(void) {
    printf("\nChoose Qualification:\n");
    printf("1. 10th Pass\n");
    printf("2. 12th Pass\n");
    return readIntInRange("Enter choice (1-2): ", 1, 2);
}

const Job* chooseJob(int qual) {
    int i, choice, count;
    const Job *arr;

    if (qual == 1) {
        arr = jobs10th; count = jobs10thCount;
        printf("\nJobs (10th):\n");
    } else {
        arr = jobs12th; count = jobs12thCount;
        printf("\nJobs (12th):\n");
    }
    for (i = 0; i < count; i++) {
        printf("%d. %s\n", i + 1, arr[i].jobName);
    }
    choice = readIntInRange("Choose job: ", 1, count);
    return &arr[choice - 1];
}

/* ===== Post-job choice menu: 1 view syllabus, 2 enroll ===== */
int postJobChoiceMenu(const Job *job) {
    int i;
    printf("\nYou selected: %s\n", job->jobName);
    printf("1. View syllabus\n");
    printf("2. Enroll this course\n");
    for (;;) {
        int c = readIntInRange("Enter choice (1-2): ", 1, 2);
        if (c == 1) {
            for (i = 0; i < job->subjectCount; i++) {
                printf("\n--- %s Syllabus ---\n", job->subjects[i].name);
                showSyllabusFile(job->subjects[i].syllabusFile);
            }
            printf("\n1. View syllabus\n2. Enroll this course\n");
            continue;
        } else {
            return 2;
        }
    }
}

/* ===== Save progress for a specific course ===== */
void saveCourseProgressToFile(const Student *s, const Course *c) {
    char filename[160];
    int i;
    sprintf(filename, "%s_%s_progress.txt", s->name, c->jobName);
    for (i = 0; filename[i]; i++) if (filename[i] == ' ') filename[i] = '_';

    FILE *f = fopen(filename, "w");
    if (!f) {
        printf("Failed to save progress.\n");
        return;
    }
    fprintf(f, "Student: %s\nCourse: %s\n", s->name, c->jobName);
    for (i = 0; i < c->subjectCount; i++) {
        fprintf(f, "%s: %d%%\n", c->subjects[i].name, c->subjects[i].progress);
    }
    fclose(f);
    printf("Progress saved to %s\n", filename);
}

/* ===== Per-course menu for enrolled students ===== */
void courseMenu(Student *s, int courseIndex) {
    Course *c = &s->courses[courseIndex];
    int choice, i;

    for (;;) {
        printf("\n=== %s | Course: %s ===\n", s->name, c->jobName);
        printf("1. Progress logger (update progress)\n");
        printf("2. Track progress\n");
        printf("3. View syllabus\n");
        printf("4. Exit to enrolled menu\n");

        choice = readIntInRange("Enter choice (1-4): ", 1, 4);

        if (choice == 1) {
            int sub, prog;
            for (i = 0; i < c->subjectCount; i++) {
                printf("%d. %s (current %d%%)\n", i + 1, c->subjects[i].name, c->subjects[i].progress);
            }
            sub = readIntInRange("Choose subject: ", 1, c->subjectCount);
            prog = readIntInRange("Enter progress (0-100): ", 0, 100);
            c->subjects[sub - 1].progress = prog;
            printf("Updated.\n");

            // ✅ Saving progress after update
            saveCourseProgressToFile(s, c);

        } else if (choice == 2) {
            int total = 0;
            for (i = 0; i < c->subjectCount; i++) {
                printf("%s: %d%%\n", c->subjects[i].name, c->subjects[i].progress);
                total += c->subjects[i].progress;
            }
            printf("Overall progress: %.1f%%\n", (c->subjectCount ? (total * 1.0 / c->subjectCount) : 0.0));
        } else if (choice == 3) {
            for (i = 0; i < c->subjectCount; i++) {
                printf("\n--- %s Syllabus ---\n", c->subjects[i].name);
                showSyllabusFile(c->subjects[i].syllabusFile);
            }
        } else {
            break;
        }
    }
}
/* ===== Enrolled student hub ===== */
void enrolledHub(Student *s) {
    for (;;) {
        int i, choice;
        printf("\n=== Enrolled Menu (%s) ===\n", s->name);
        if (s->courseCount == 0) {
            printf("You have no courses enrolled.\n");
        } else {
            printf("Your courses:\n");
            for (i = 0; i < s->courseCount; i++) {
                printf("%d. %s\n", i + 1, s->courses[i].jobName);
            }
        }
        printf("Options:\n");
        printf("1. Progress logger / Update progress (choose a course)\n");
        printf("2. Track progress (choose a course)\n");
        printf("3. View syllabus (choose a course)\n");
        printf("4. Exit to name prompt\n");
        printf("5. Add another one\n");

        choice = readIntInRange("Enter choice (1-5): ", 1, 5);

        if (choice >= 1 && choice <= 3) {
            if (s->courseCount == 0) {
                printf("No courses to operate on.\n");
                continue;
            }
            printf("Select a course:\n");
            for (i = 0; i < s->courseCount; i++) printf("%d. %s\n", i + 1, s->courses[i].jobName);
            i = readIntInRange("Enter course number: ", 1, s->courseCount) - 1;

            if (choice == 1) {
                courseMenu(s, i);
            } else if (choice == 2) {
                int j, total = 0;
                Course *c = &s->courses[i];
                for (j = 0; j < c->subjectCount; j++) {
                    printf("%s: %d%%\n", c->subjects[j].name, c->subjects[j].progress);
                    total += c->subjects[j].progress;
                }
                printf("Overall progress: %.1f%%\n", (c->subjectCount ? (total * 1.0 / c->subjectCount) : 0.0));
            } else if (choice == 3) {
                int j;
                Course *c = &s->courses[i];
                for (j = 0; j < c->subjectCount; j++) {
                    printf("\n--- %s Syllabus ---\n", c->subjects[j].name);
                    showSyllabusFile(c->subjects[j].syllabusFile);
                }
            } 
        } else if (choice == 4) {
            break;
        } else if (choice == 5) {
            if (s->courseCount >= MAX_COURSES) {
                printf("You reached the maximum number of courses.\n");
                continue;
            }
            int q = chooseQualification();
            const Job *job = chooseJob(q);
            if (postJobChoiceMenu(job) == 2) {
                makeCourseFromJob(job, &s->courses[s->courseCount]);
                s->courseCount++;
                printf("Enrollment added.\n");
            }
        }
    }
}
/* ===== Main: Enter-name loop ===== */
int main(void) {
    for (;;) {
        char name[MAX_NAME];
        readLine("\nEnter your name (or type EXIT to quit): ", name, sizeof(name));
        if (strcmp(name, "EXIT") == 0 || strcmp(name, "exit") == 0) break;

        int idx = findStudentIndexByName(name);
        if (idx >= 0) {
            enrolledHub(&students[idx]);
        } else {
            int q = chooseQualification();
            const Job *job = chooseJob(q);
            int action = postJobChoiceMenu(job);

            if (action == 2) {
                idx = addNewStudent(name);
                if (idx < 0) {
                    printf("System is full. Cannot enroll.\n");
                    continue;
                }
                makeCourseFromJob(job, &students[idx].courses[0]);
                students[idx].courseCount = 1;
                printf("Enrollment successful for %s in %s.\n", name, job->jobName);
            }
        }
    }

    printf("Goodbye.\n");
    return 0;
}
